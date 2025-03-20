#include "audio_scene.h"
#include "audio_hw.h"
#include "audio_decoder.h"
#include "dsp_mem.h"

#include "audio_a2dp_sink.h"

#define DECODER_DATA_READY_THD              4
#define DECODER_DATA_MAX_THD                10

#define TONE_RAW_DATA_BUFFER_SIZE           128

typedef struct {
    audio_hw_t *hw;
    audio_decoder_t *decoder;
//    audio_decoder_t *decoder_tone;
    audio_decoder_output_t *decoder_output_to_hw;
//    audio_scene_decoder_req_raw_cb tone_req_raw_cb;
    
    uint32_t start_thd;
    uint8_t raw_frame_nb_in_list;
    struct co_list raw_frame_list;
    
//    uint8_t *tone_raw_data;         // used to save raw tone frame
} audio_a2dp_sink_env_t;

static audio_scene_t *a2dp_sink_scene = NULL;

static void hw_request_pcm_handler(void *pcm, uint32_t samples, uint8_t channels)
{
    uint32_t avaliable_data;
    uint32_t missed_sample;
    audio_a2dp_sink_env_t *env = a2dp_sink_scene->env;
    
    /* request PCM data from audio decoder */
    avaliable_data = audio_decoder_get_pcm(env->decoder_output_to_hw, pcm, samples, channels);
    missed_sample = audio_decoder_get_missed_sample_cnt(env->decoder_output_to_hw);
    if(missed_sample){
        audio_decoder_stop(env->decoder);
        audio_decoder_clear_missed_sample_cnt(env->decoder_output_to_hw);
        if((env->start_thd == 0) && (env->raw_frame_nb_in_list < DECODER_DATA_READY_THD)){
            env->start_thd = DECODER_DATA_READY_THD;
        }
    }
}

static void decoder_request_raw_data_handler(audio_decoder_t *decoder, uint8_t event)
{
    if (event == AUDIO_DECODER_EVENT_REQ_RAW_DATA) {
        audio_scene_evt_req_encoded_frame_t *evt = (void *)pvPortMalloc(sizeof(audio_scene_evt_req_encoded_frame_t));
        
        if (evt) {
            evt->evt.type = AUDIO_SCENE_EVT_TYPE_REQ_ENCODED_FRAME;
            evt->evt.scene = a2dp_sink_scene;
            evt->decoder = decoder;
            audio_scene_send_event(&evt->evt);
        }
    }
}

static audio_scene_t *allocate(void *_param)
{
    audio_scene_t *scene = pvPortMalloc(sizeof(audio_scene_t));

    if (scene) {
        /* init a2dp sink enviroment */
        audio_a2dp_sink_env_t *env = pvPortMalloc(sizeof(audio_a2dp_sink_env_t));
        if (env == NULL) {
            vPortFree(scene);
            return NULL;
        }
        
        /* allocate buffer to save a2dp sink parameters */
        audio_a2dp_sink_param_t *param = pvPortMalloc(sizeof(audio_a2dp_sink_param_t));
        if (param == NULL) {
            vPortFree(env);
            vPortFree(scene);
            return NULL;
        }
        
        scene->env = env;
        memcpy((void *)param, _param, sizeof(audio_a2dp_sink_param_t));
        scene->param = param;
        scene->op = &audio_a2dp_sink_operator;
    }
    
    return scene;
}

static void init(audio_scene_t *scene)
{
    audio_a2dp_sink_env_t *env = scene->env;
    audio_a2dp_sink_param_t *param = scene->param;
    
    a2dp_sink_scene = scene;
    
    env->start_thd = DECODER_DATA_READY_THD;
    env->raw_frame_nb_in_list = 0;
    co_list_init(&env->raw_frame_list);
    
    /* initialize audio decoder module */
    audio_decoder_init(param->channels, param->sample_rate);
    /* create and start a decoder */
    env->decoder = audio_decoder_add(param->decoder_type, &param->decoder_param, decoder_request_raw_data_handler);
    /* add an output to initialized audio decoder module */
    env->decoder_output_to_hw = audio_decoder_output_add(true, param->channels);
    
    /* initialize audio hardware */
    env->hw = audio_hw_create(param->hw_type, hw_request_pcm_handler, param->hw_base_addr, AUDIO_HW_DIR_OUT, param->sample_rate, param->channels);
}

static void destroy(audio_scene_t *scene)
{
    audio_a2dp_sink_env_t *env = scene->env;
    
    audio_hw_destroy(env->hw);
    audio_decoder_destroy();
    
    audio_data_element_t *elt = (void *)co_list_pop_front(&env->raw_frame_list);
    while (elt) {
        vPortFree(elt);
        elt = (void *)co_list_pop_front(&env->raw_frame_list);
    }
    
    vPortFree(scene->env);
    vPortFree(scene->param);
    vPortFree(scene);
    a2dp_sink_scene = NULL;
}

static void event_handler(audio_scene_t *scene, audio_scene_evt_t *evt)
{
    audio_a2dp_sink_env_t *env = scene->env;
    audio_a2dp_sink_param_t *param = scene->param;

    switch(evt->type) {
        case AUDIO_SCENE_EVT_TYPE_REQ_ENCODED_FRAME:
            {
                uint32_t length = 0;
                audio_scene_evt_req_encoded_frame_t *_evt = (void *)evt;
                if (_evt->decoder == env->decoder) {
                    if (audio_decoder_decode(env->decoder,
                                                NULL, &length) != AUDIO_RET_OUTPUT_ALMOTE_FULL) {
                        if (env->start_thd == 0) {
                            audio_data_element_t *elt;
                            int ret;
                            elt = (void *)co_list_pick(&env->raw_frame_list);
                            while (elt) {
                                uint32_t length = elt->length - elt->offset;
                                ret = audio_decoder_decode(env->decoder, &elt->buffer[elt->offset], &length);
                                elt->offset += length;
                                if (elt->length == elt->offset) {
                                    elt = (void *)co_list_pop_front(&env->raw_frame_list);
//                                    fputc(elt->seq, NULL);
                                    env->raw_frame_nb_in_list--;
                                    vPortFree(elt);
                                    elt = (void *)co_list_pick(&env->raw_frame_list);
                                }
                                if (ret == AUDIO_RET_OUTPUT_ALMOTE_FULL) {
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            break;
        case AUDIO_SCENE_EVT_TYPE_RECV_ENCODED_FRAME:
            {
                audio_scene_evt_recv_encoded_data_t *_evt = (void *)evt;
                if(env->raw_frame_nb_in_list > DECODER_DATA_MAX_THD){
                    /*这里判断并开启sink解码器是为了防止其在卡顿时接收足够压缩数据重启被再次立即关闭导致的无声情况，
                    出现上述情况的原因是在重启前很近时间有提示音的解码事件一直占据audio task任务导致sink无法解码数据，
                    解码模块PCM buffer的wr_ptr无法更新，在hw_request_pcm_handler被再次关闭*/
                    if(!audio_decoder_is_started(env->decoder))
                    {
                        audio_decoder_start(env->decoder);
                    }
                    vPortFree(_evt->raw_frame);
                }
                else{
                    co_list_push_back(&env->raw_frame_list, &_evt->raw_frame->hdr);
                    env->raw_frame_nb_in_list++;
                    if (env->start_thd) {
                        env->start_thd--;
                        if (env->start_thd == 0) {
                            audio_decoder_start(env->decoder);
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
    if ((scene == NULL) || (scene != a2dp_sink_scene)) {
        return false;
    }
    else {
        audio_a2dp_sink_env_t *env = scene->env;
        return audio_decoder_is_started(env->decoder);
    }
}

void audio_a2dp_sink_handle_missed_samples(void *arg, uint32_t samples)
{
    audio_a2dp_sink_env_t *env = a2dp_sink_scene->env;
    GLOBAL_INT_DISABLE();
    env->decoder_output_to_hw->missed_samples -= samples;
    GLOBAL_INT_RESTORE();
}

audio_scene_operator_t audio_a2dp_sink_operator = {
    .allocate = allocate,
    .init = init,
    .destroy = destroy,
    .event_handler = event_handler,
    .decoder_started = decoder_started,
    .support_tone = true,
};
