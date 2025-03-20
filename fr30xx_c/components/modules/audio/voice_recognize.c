#include "audio_scene.h"
#include "audio_rpmsg.h"
#include "audio_hw.h"
#include "audio_encoder.h"
#include "dsp_mem.h"

#include "voice_recognize.h"

#define STORE_MIC_PCM_DATA_SAMPLES      128

typedef struct {
    audio_hw_t *hw;
    audio_hw_output_t *hw_output;
    
    uint16_t *buffer;
} voice_recognize_env_t;

static audio_scene_t *voice_recognize_scene = NULL;

static void hw_receive_pcm(uint32_t samples)
{
    audio_scene_evt_hw_in_new_samples_t *evt = (void *)pvPortMalloc(sizeof(audio_scene_evt_hw_in_new_samples_t));
    
    if (evt) {
        evt->evt.type = AUDIO_SCENE_EVT_TYPE_HW_IN_NEW_SAMPLES;
        evt->evt.scene = voice_recognize_scene;
        evt->adc_new_samples = samples;
        audio_scene_send_event(&evt->evt);
    }
}

static audio_scene_t *allocate(void *_param)
{
    audio_scene_t *scene = pvPortMalloc(sizeof(audio_scene_t));

    if (scene) {
        voice_recognize_env_t *env = pvPortMalloc(sizeof(voice_recognize_env_t));
        if (env == NULL) {
            vPortFree(scene);
            return NULL;
        }
        
        voice_recognize_param_t *param = pvPortMalloc(sizeof(voice_recognize_param_t));
        if (param == NULL) {
            vPortFree(env);
            vPortFree(scene);
            return NULL;
        }
        
        scene->env = env;
        memcpy((void *)param, _param, sizeof(voice_recognize_param_t));
        scene->param = param;
        scene->op = &voice_recognize_operator;
    }
    
    return scene;
}

static void init(audio_scene_t *scene)
{
    voice_recognize_env_t *env = scene->env;
    voice_recognize_param_t *param = scene->param;
    
    voice_recognize_scene = scene;

    env->buffer = pvPortMalloc(STORE_MIC_PCM_DATA_SAMPLES * sizeof(uint16_t) * param->channels);
    
    env->hw = audio_hw_create(param->hw_type, NULL, param->hw_base_addr, AUDIO_HW_DIR_IN, param->sample_rate, param->channels);
    env->hw_output = audio_hw_output_add(env->hw, hw_receive_pcm);
}

static void destroy(audio_scene_t *scene)
{
    voice_recognize_env_t *env = scene->env;
    
    audio_hw_destroy(env->hw);
    
    vPortFree(env->buffer);
    vPortFree(scene->env);
    vPortFree(scene->param);
    vPortFree(scene);
    voice_recognize_scene = NULL;
}

static void event_handler(audio_scene_t *scene, audio_scene_evt_t *evt)
{
    voice_recognize_env_t *env = scene->env;
    voice_recognize_param_t *param = scene->param;

    switch(evt->type) {
        case AUDIO_SCENE_EVT_TYPE_HW_IN_NEW_SAMPLES:
            {
                int encoded_frame_count;
                audio_scene_evt_hw_in_new_samples_t *_evt = (void *)evt;
                uint32_t adc_new_samples = _evt->adc_new_samples;
                while(adc_new_samples) {
                    uint32_t samples = adc_new_samples > STORE_MIC_PCM_DATA_SAMPLES ? STORE_MIC_PCM_DATA_SAMPLES : adc_new_samples;
                    samples = audio_hw_read_pcm(env->hw_output, env->buffer, samples, param->channels);
//                    voice_recognize_launch((int16_t *)env->buffer, samples);
                    adc_new_samples -= samples;
                }
            }
            break;
        default:
            break;
    }
}

audio_scene_operator_t voice_recognize_operator = {
    .allocate = allocate,
    .init = init,
    .destroy = destroy,
    .event_handler = event_handler,
    .decoder_started = NULL,
    .support_tone = false,
};
