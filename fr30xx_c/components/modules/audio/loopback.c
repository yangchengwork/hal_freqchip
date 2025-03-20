#include "audio_scene.h"
#include "audio_rpmsg.h"
#include "audio_hw.h"
#include "audio_encoder.h"
#include "dsp_mem.h"

#include "loopback.h"

#define STORE_IN_PCM_DATA_SAMPLES      1024

typedef struct {
    audio_hw_t *out_hw;
    audio_hw_output_t *hw_output;
    
    audio_hw_t *in_hw;

    uint8_t *buffer;
    uint32_t wr_ptr;
    uint32_t rd_ptr;
} loopback_env_t;

static audio_scene_t *loopback_scene = NULL;

static void hw_receive_pcm(uint32_t samples)
{
    audio_scene_evt_hw_in_new_samples_t *evt = (void *)pvPortMalloc(sizeof(audio_scene_evt_hw_in_new_samples_t));
    
    if (evt) {
        evt->evt.type = AUDIO_SCENE_EVT_TYPE_HW_IN_NEW_SAMPLES;
        evt->evt.scene = loopback_scene;
        evt->adc_new_samples = samples;
        audio_scene_send_event(&evt->evt);
    }
}

static void hw_request_pcm(void *pcm, uint32_t samples, uint8_t channels)
{    
    if (loopback_scene == NULL) {
        return;
    }
    loopback_env_t *_env = loopback_scene->env;
    loopback_param_t *param = loopback_scene->param;
    
    uint32_t tail_samples = STORE_IN_PCM_DATA_SAMPLES - _env->rd_ptr;
    if (tail_samples > samples) {
        memcpy(pcm, &_env->buffer[_env->rd_ptr * param->channels * sizeof(int16_t)], samples * param->channels * sizeof(int16_t));
        _env->rd_ptr += samples;
        samples = 0;
    }
    else {
        memcpy(pcm, &_env->buffer[_env->rd_ptr * param->channels * sizeof(int16_t)], tail_samples * param->channels * sizeof(int16_t));
        _env->rd_ptr = 0;
        samples -= tail_samples;
        
        if (samples) {
            memcpy(pcm, &_env->buffer[0], samples * param->channels * sizeof(int16_t));
            _env->rd_ptr += samples;
        }
    }
}

static audio_scene_t *allocate(void *_param)
{
    audio_scene_t *scene = pvPortMalloc(sizeof(audio_scene_t));

    if (scene) {
        loopback_env_t *env = pvPortMalloc(sizeof(loopback_env_t));
        if (env == NULL) {
            vPortFree(scene);
            return NULL;
        }
        
        loopback_param_t *param = pvPortMalloc(sizeof(loopback_param_t));
        if (param == NULL) {
            vPortFree(env);
            vPortFree(scene);
            return NULL;
        }
        
        scene->env = env;
        memcpy((void *)param, _param, sizeof(loopback_param_t));
        scene->param = param;
        scene->op = &loopback_operator;
    }
    
    return scene;
}

static void init(audio_scene_t *scene)
{
    bool inout_mode = false;
    loopback_env_t *env = scene->env;
    loopback_param_t *param = scene->param;
    
    loopback_scene = scene;

    env->buffer = pvPortMalloc(STORE_IN_PCM_DATA_SAMPLES * sizeof(uint16_t) * param->channels);
    env->wr_ptr = 0;
    env->rd_ptr = STORE_IN_PCM_DATA_SAMPLES / 2;
    
    if (param->in_hw_type == param->out_hw_type) {
        if ((param->in_hw_type == AUDIO_HW_TYPE_CODEC)
                || ((param->in_hw_type == AUDIO_HW_TYPE_I2S) && (param->in_hw_base_addr == param->out_hw_base_addr))){
            inout_mode = true;
        }
    }
    
    if (inout_mode) {
        env->in_hw = audio_hw_create(param->in_hw_type, hw_request_pcm, param->in_hw_base_addr, AUDIO_HW_DIR_INOUT, param->sample_rate, param->channels);
        env->out_hw = env->in_hw;
    }
    else {
        env->in_hw = audio_hw_create(param->in_hw_type, NULL, param->in_hw_base_addr, AUDIO_HW_DIR_IN, param->sample_rate, param->channels);
        env->out_hw = audio_hw_create(param->out_hw_type, hw_request_pcm, param->out_hw_base_addr, AUDIO_HW_DIR_OUT, param->sample_rate, param->channels);
    }
    env->hw_output = audio_hw_output_add(env->in_hw, hw_receive_pcm);
}

static void destroy(audio_scene_t *scene)
{
    loopback_env_t *env = scene->env;
    
    if (env->in_hw != env->out_hw) {
        audio_hw_destroy(env->out_hw);
    }
    audio_hw_destroy(env->in_hw);
    
    vPortFree(env->buffer);
    vPortFree(scene->env);
    vPortFree(scene->param);
    vPortFree(scene);
    loopback_scene = NULL;
}

static void event_handler(audio_scene_t *scene, audio_scene_evt_t *evt)
{
    loopback_env_t *env = scene->env;
    loopback_param_t *param = scene->param;

    switch(evt->type) {
        case AUDIO_SCENE_EVT_TYPE_HW_IN_NEW_SAMPLES:
            {
                int encoded_frame_count;
                audio_scene_evt_hw_in_new_samples_t *_evt = (void *)evt;
                uint32_t adc_new_samples = _evt->adc_new_samples;
                while(adc_new_samples) {
                    uint32_t samples = adc_new_samples > (STORE_IN_PCM_DATA_SAMPLES - env->wr_ptr) ? (STORE_IN_PCM_DATA_SAMPLES - env->wr_ptr) : adc_new_samples;
                    samples = audio_hw_read_pcm(env->hw_output, env->buffer + env->wr_ptr * param->channels * sizeof(int16_t), samples, param->channels);
                    env->wr_ptr += samples;
                    if (env->wr_ptr >= STORE_IN_PCM_DATA_SAMPLES) {
                        env->wr_ptr = 0;
                    }
                    adc_new_samples -= samples;
                }
            }
            break;
        default:
            break;
    }
}

audio_scene_operator_t loopback_operator = {
    .allocate = allocate,
    .init = init,
    .destroy = destroy,
    .event_handler = event_handler,
    .decoder_started = NULL,
    .support_tone = false,
};
