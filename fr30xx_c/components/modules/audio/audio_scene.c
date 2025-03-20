#include <assert.h>

#include "audio_scene.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "co_list.h"
#include "codec.h"
#include "algorithm.h"
#include "dsp_mem.h"

#define TONE_RAW_DATA_BUFFER_SIZE           128

static struct co_list evt_list;
static TaskHandle_t audio_scene_task_handle;

/* 
 * 当前场景如果只是提示音，那么audio_scene和audio_tone_scene是一样的，
 * 如果当前主场景是其他的，那么这两个值不一样。
 * 提示音现在当成一种特殊的场景，如果当前没有其他场景，那么就创建成普
 * 通场景，如果有就当成其他场景的附属场景。
 */
static audio_scene_t *audio_scene = NULL;
static audio_scene_t *audio_tone_scene = NULL;
static SemaphoreHandle_t sema_audio_scene = ((void *)0);

typedef struct  {
    audio_hw_t *hw;
    audio_decoder_t *decoder;
    audio_decoder_output_t *decoder_output_to_hw;
    
    uint8_t *raw_data;         // used to save raw tone frame
} audio_tone_env_t;

static audio_scene_t *allocate(void *_param);
static void init(audio_scene_t *scene);
static void destroy(audio_scene_t *scene);
static void event_handler(audio_scene_t *scene, audio_scene_evt_t *evt);
static bool decoder_started(audio_scene_t *scene);

struct audio_scene_operator audio_tone_operator = {
    .allocate = allocate,
    .init = init,
    .destroy = destroy,
    .decoder_started = decoder_started,
    .support_tone = false,
    .event_handler = event_handler,
};

static void decoder_request_raw_data_handler(audio_decoder_t *decoder, uint8_t event)
{
    if (event == AUDIO_DECODER_EVENT_REQ_RAW_DATA) {
        audio_scene_evt_req_encoded_frame_t *evt = (void *)pvPortMalloc(sizeof(audio_scene_evt_req_encoded_frame_t));
        
        if (evt) {
            evt->evt.type = AUDIO_SCENE_EVT_TYPE_REQ_ENCODED_FRAME;
            evt->evt.scene = audio_tone_scene;
            evt->decoder = decoder;
            audio_scene_send_event(&evt->evt);
        }
    }
    else if (event == AUDIO_DECODER_EVENT_PCM_CONSUMED)
    {
        if (xPortIsInsideInterrupt())
        {
            BaseType_t xTaskWokenByReceive = pdFALSE;
            xSemaphoreTakeFromISR(sema_audio_scene, &xTaskWokenByReceive);
            portEND_SWITCHING_ISR(xTaskWokenByReceive);
        }
        else{
            xSemaphoreTake(sema_audio_scene, portMAX_DELAY);
        }
        audio_scene_evt_t *evt;
        evt = (void *)pvPortMalloc(sizeof(audio_scene_evt_t));
        evt->type = AUDIO_SCENE_EVT_TYPE_DESTROY;
        evt->scene = audio_tone_scene;
        audio_scene_send_event(evt);
    }
}

static void hw_request_pcm_handler(void *pcm, uint32_t samples, uint8_t channels)
{
    audio_tone_env_t *env = audio_tone_scene->env;
    
    /* request PCM data from audio decoder */
    audio_decoder_get_pcm(env->decoder_output_to_hw, pcm, samples, channels);
}

static audio_scene_t *allocate(void *_param)
{
    audio_scene_t *scene = pvPortMalloc(sizeof(audio_scene_t));

    if (scene) {
        /* init a2dp sink enviroment */
        audio_tone_env_t *env = pvPortMalloc(sizeof(audio_tone_env_t));
        if (env == NULL) {
            vPortFree(scene);
            return NULL;
        }
        memset(env, 0, sizeof(audio_tone_env_t));
        
        /* allocate buffer to save a2dp sink parameters */
        audio_tone_param_t *param = pvPortMalloc(sizeof(audio_tone_param_t));
        if (param == NULL) {
            vPortFree(env);
            vPortFree(scene);
            return NULL;
        }
        
        scene->env = env;
        memcpy((void *)param, _param, sizeof(audio_tone_param_t));
        scene->param = param;
        scene->op = &audio_tone_operator;
    }
    
    return scene;
}

static void init(audio_scene_t *scene)
{
    audio_tone_env_t *env = scene->env;
    audio_tone_param_t *param = scene->param;
    
    audio_tone_scene = scene;
    
    env->raw_data = pvPortMalloc(TONE_RAW_DATA_BUFFER_SIZE);
    
    /* 这两个值如果一样，那么就意味着提示音是主场景 */
    if (audio_scene == audio_tone_scene) {
        /* initialize audio decoder module */
        audio_decoder_init(param->channels, param->sample_rate);
        /* create and start a decoder */
        env->decoder = audio_decoder_add(param->audio_type, &param->decoder_param, decoder_request_raw_data_handler);
        audio_decoder_start(env->decoder);
        /* add an output to initialized audio decoder module */
        env->decoder_output_to_hw = audio_decoder_output_add(true, param->channels);
        
        /* initialize audio hardware */
        env->hw = audio_hw_create(param->hw_type, hw_request_pcm_handler, param->hw_base_addr, AUDIO_HW_DIR_OUT, param->sample_rate, param->channels);
    }
    else {
        /* create and start a decoder */
        env->decoder = audio_decoder_add(param->audio_type, &param->decoder_param, decoder_request_raw_data_handler);
        audio_decoder_start(env->decoder);
    }
}

static void destroy(audio_scene_t *scene)
{
    if(scene != audio_tone_scene)
    {
        return;
    }
    audio_tone_env_t *env = scene->env;
    
    if (audio_scene == audio_tone_scene) {
        audio_hw_destroy(env->hw);
        audio_decoder_destroy();
    }
    else {
        if(audio_tone_scene)
        {
        	audio_decoder_remove(env->decoder);
        }
    }

    vPortFree(env->raw_data);
    vPortFree(scene->env);
    vPortFree(scene->param);
    vPortFree(scene);
    audio_tone_scene = NULL;
}

static void event_handler(audio_scene_t *scene, audio_scene_evt_t *evt)
{
    audio_tone_env_t *env = scene->env;
    audio_tone_param_t *param = scene->param;

    switch(evt->type) {
        case AUDIO_SCENE_EVT_TYPE_REQ_ENCODED_FRAME:
            {
                uint32_t length = 0;
                audio_scene_evt_req_encoded_frame_t *_evt = (void *)evt;
                if (_evt->decoder == env->decoder) {
                    if (audio_decoder_decode(env->decoder,
                                                NULL, &length) != AUDIO_RET_OUTPUT_ALMOTE_FULL) {
                        if(param->req_raw_cb){
                            int ret;
                            do {
                                ret = 0;
                                length = param->req_raw_cb(env->raw_data, TONE_RAW_DATA_BUFFER_SIZE);
                                if (length) {
                                    ret = audio_decoder_decode(env->decoder, env->raw_data, &length);
                                }
                                else {
                                    length = AUDIO_SPECIAL_LENGTH_FOR_INPUT_OVER;
                                    audio_decoder_decode(env->decoder, NULL, &length);
                                    break;
                                }
                            } while(ret == AUDIO_RET_NEED_MORE);
                        }
                    }
                }
            }
            break;
        default:
            break;
    }
}

static bool decoder_started(audio_scene_t *scene)
{
    if ((scene == NULL) || (scene != audio_tone_scene)) {
        return false;
    }
    else {
        audio_tone_env_t *env = scene->env;
        return audio_decoder_is_started(env->decoder);
    }
}

static void audio_scene_task(void *arg)
{
    audio_scene_evt_t *evt;
    audio_scene_t *scene;
    sema_audio_scene = xSemaphoreCreateBinary();
    xSemaphoreGive(sema_audio_scene);
    while (1) {
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        GLOBAL_INT_DISABLE();
        evt = (void *)co_list_pop_front(&evt_list);
        GLOBAL_INT_RESTORE();

        if (evt) {
            switch (evt->type) {
                case AUDIO_SCENE_EVT_TYPE_CREATE:
                    if (evt->scene != NULL) {
                        evt->scene->op->init(evt->scene);
                    }
                    else {
                        assert(0);
                    }
                    xSemaphoreGive(sema_audio_scene);
                    break;
                case AUDIO_SCENE_EVT_TYPE_DESTROY:
                    if (evt->scene != NULL) {
                        /* 
                         * 删除场景的时候，需要检测一下提示音场景的状态：如果存在提示音，并且待
                         * 删除的场景是其他的主场景，那么就需要连带把提示音场景删掉
                         */
                        audio_scene_tone_destroyed_cb cb = NULL;
                        if(evt->scene == audio_tone_scene){
                            audio_tone_param_t *tmp = audio_tone_scene->param;
                            cb = tmp->tone_destroyed_cb;
                        }

                        if ((audio_tone_scene != NULL) && (evt->scene != audio_tone_scene)) {
                            destroy(audio_tone_scene);
                        }
                        evt->scene->op->destroy(evt->scene);
                        if (evt->scene == audio_scene) {
                            audio_scene = NULL;
                        }
                        if(cb){
                            cb();
                        }
                    }
                    // else {
                    //     assert(0);
                    // }
                    xSemaphoreGive(sema_audio_scene);
                    break;
                default:
                    if ((evt->scene == audio_scene) || (evt->scene == audio_tone_scene))
                    {
                        evt->scene->op->event_handler(evt->scene, evt);
                    }
                    else {
                        if (evt->type == AUDIO_SCENE_EVT_TYPE_RECV_ENCODED_FRAME) {
                            audio_scene_evt_recv_encoded_data_t *_evt = (void *)evt;
                            vPortFree(_evt->raw_frame);
                        }
                    }
                    break;
            }
            
            vPortFree(evt);
        }
    }
}

void audio_scene_init(uint32_t stack_size, uint8_t priority)
{
    co_list_init(&evt_list);

    xTaskCreate(audio_scene_task, "AUDIO_SCENE_TASK", stack_size, NULL, priority, &audio_scene_task_handle);
}

audio_scene_t *audio_scene_create(audio_scene_operator_t *op, void *param)
{
    xSemaphoreTake(sema_audio_scene, portMAX_DELAY);

    if (audio_scene != NULL) {
        audio_scene_evt_t *evt;
        evt = (void *)pvPortMalloc(sizeof(audio_scene_evt_t));
        evt->type = AUDIO_SCENE_EVT_TYPE_DESTROY;
        evt->scene = audio_scene;
        audio_scene_send_event(evt);
    }
    else if (audio_tone_scene != NULL) {
        audio_scene_evt_t *evt;
        evt = (void *)pvPortMalloc(sizeof(audio_scene_evt_t));
        evt->type = AUDIO_SCENE_EVT_TYPE_DESTROY;
        evt->scene = audio_tone_scene;//audio_scene;
        audio_scene_send_event(evt);
    }
    else{
        xSemaphoreGive(sema_audio_scene);
    }
    audio_scene = op->allocate(param);
    if (audio_scene) {
        xSemaphoreTake(sema_audio_scene, portMAX_DELAY);
        audio_scene_evt_t *evt;
        evt = (void *)pvPortMalloc(sizeof(audio_scene_evt_t));
        evt->type = AUDIO_SCENE_EVT_TYPE_CREATE;
        evt->scene = audio_scene;
        audio_scene_send_event(evt);
    }
    
    return audio_scene;
}

void audio_scene_destroy(audio_scene_t *scene)
{
    if ((scene == audio_scene) || (scene == audio_tone_scene)) {
        xSemaphoreTake(sema_audio_scene, portMAX_DELAY);
        audio_scene_evt_t *evt;
        evt = (void *)pvPortMalloc(sizeof(audio_scene_evt_t));
        evt->type = AUDIO_SCENE_EVT_TYPE_DESTROY;
        evt->scene = scene;
        audio_scene_send_event(evt);
    }
}

void audio_scene_send_event(audio_scene_evt_t *evt)
{
    if (evt == NULL) {
        return;
    }
    
    GLOBAL_INT_DISABLE();
    co_list_push_back(&evt_list, &evt->hdr);
    GLOBAL_INT_RESTORE();
    
    if(xPortIsInsideInterrupt()) {
        vTaskNotifyGiveFromISR(audio_scene_task_handle, NULL);
    }
    else {
        xTaskNotifyGive(audio_scene_task_handle);
    }
}

void audio_scene_recv_encoded_data(audio_scene_t *scene, bool valid, const uint8_t *buffer, uint32_t length)
{
    audio_data_element_t *elt;
    static uint8_t seq = 0;
        
    audio_scene_evt_recv_encoded_data_t *evt;
    evt = (void *)pvPortMalloc(sizeof(audio_scene_evt_recv_encoded_data_t));
    evt->evt.type = AUDIO_SCENE_EVT_TYPE_RECV_ENCODED_FRAME;
    evt->evt.scene = scene;
    if (valid) {
        elt = (void *)pvPortMalloc(sizeof(audio_data_element_t) + length);
        elt->valid = true;
        elt->length = length;
        elt->offset = 0;
        memcpy((void *)&elt->buffer[0], buffer, length);
    }
    else {
        elt = (void *)pvPortMalloc(sizeof(audio_data_element_t));
        elt->valid = false;
        elt->length = AUDIO_SPECIAL_LENGTH_FOR_PLC;
        elt->offset = 0;
    }
//    fputc(seq, NULL);
    evt->raw_frame = elt;
    audio_scene_send_event(&evt->evt);
}

audio_scene_t *audio_scene_tone_play(audio_tone_param_t *param)
{
    xSemaphoreTake(sema_audio_scene, portMAX_DELAY);

    if (audio_scene) {
            if(audio_tone_scene == NULL)
            {
                if (audio_scene->op->support_tone) {
                    /* check whether an tone is ongoing  */
                    audio_tone_scene = allocate(param);
                    audio_scene_evt_t *evt;
                    evt = (void *)pvPortMalloc(sizeof(audio_scene_evt_t));
                    evt->type = AUDIO_SCENE_EVT_TYPE_CREATE;
                    evt->scene = audio_tone_scene;
                    audio_scene_send_event(evt);
                }else{
                    xSemaphoreGive(sema_audio_scene);
                }
            }else{
                xSemaphoreGive(sema_audio_scene);
            }
    }
    else {
        xSemaphoreGive(sema_audio_scene);
        /* create scene for tone */
        if(param != NULL){
            audio_tone_scene = audio_scene_create(&audio_tone_operator, param);
        }
    }
    return audio_scene;
}

void audio_scene_tone_stop(bool immediate)
{
    xSemaphoreTake(sema_audio_scene, portMAX_DELAY);
    if (audio_tone_scene){
        if(immediate){
            audio_scene_evt_t *evt;
            evt = (void *)pvPortMalloc(sizeof(audio_scene_evt_t));
            evt->type = AUDIO_SCENE_EVT_TYPE_DESTROY;
            evt->scene = audio_tone_scene;
            audio_scene_send_event(evt);
        }
        else {
            xSemaphoreGive(sema_audio_scene);
        }
    }
    else {
        xSemaphoreGive(sema_audio_scene);
    }
}

bool audio_scene_decoder_started(audio_scene_t *scene)
{
    if (scene && scene->op->decoder_started) {
        return scene->op->decoder_started(scene);
    }
    else {
        return false;
    }
}
