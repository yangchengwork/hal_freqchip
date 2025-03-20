#include "audio_scene.h"
#include "audio_hw.h"
#include "audio_encoder.h"
#include "dsp_mem.h"

#include "recorder.h"

#define STORE_MIC_PCM_DATA_SAMPLES      128

typedef struct {
    audio_hw_t *hw;
    audio_hw_output_t *hw_output;
    audio_encoder_t *encoder;
    
    /* used to store mic PCM data */
    uint16_t *buffer;
} recorder_env_t;

static audio_scene_t *recoder_scene = NULL;

static void hw_receive_pcm(uint32_t samples)
{
#if 1
    audio_scene_evt_hw_in_new_samples_t *evt = (void *)pvPortMalloc(sizeof(audio_scene_evt_hw_in_new_samples_t));
    
    if (evt) {
        evt->evt.type = AUDIO_SCENE_EVT_TYPE_HW_IN_NEW_SAMPLES;
        evt->evt.scene = recoder_scene;
        evt->adc_new_samples = samples;
        audio_scene_send_event(&evt->evt);
    }
#else
    recorder_env_t *env = recoder_scene->env;
    recorder_param_t *param = recoder_scene->param;
    int encoded_frame_count;
    uint32_t adc_new_samples = samples;
    while(adc_new_samples) {
        uint32_t samples = adc_new_samples > STORE_MIC_PCM_DATA_SAMPLES ? STORE_MIC_PCM_DATA_SAMPLES : adc_new_samples;
        samples = audio_hw_read_pcm(env->hw_output, env->buffer, samples, param->channels);
        adc_new_samples -= samples;
        
        uint32_t *ptr = (void *)env->buffer;
        for(uint32_t i=0;i<samples;)
        {
            char *hex2char = "0123456789abcdef";
//            fputc(hex2char[(ptr[i]>>28)&0xf], NULL);
//            fputc(hex2char[(ptr[i]>>24)&0xf], NULL);
//            fputc(hex2char[(ptr[i]>>20)&0xf], NULL);
//            fputc(hex2char[(ptr[i]>>16)&0xf], NULL);
            fputc(hex2char[(ptr[i]>>12)&0xf], NULL);
            fputc(hex2char[(ptr[i]>>8)&0xf], NULL);
            fputc(hex2char[(ptr[i]>>4)&0xf], NULL);
            fputc(hex2char[(ptr[i]>>0)&0xf], NULL);
            i+=4;
        }
        printf("\n");
    }
#endif
}

static audio_scene_t *allocate(void *_param)
{
    audio_scene_t *scene = pvPortMalloc(sizeof(audio_scene_t));

    if (scene) {
        recorder_env_t *env = pvPortMalloc(sizeof(recorder_env_t));
        if (env == NULL) {
            vPortFree(scene);
            return NULL;
        }
        
        recorder_param_t *param = pvPortMalloc(sizeof(recorder_param_t));
        if (param == NULL) {
            vPortFree(env);
            vPortFree(scene);
            return NULL;
        }
        
        scene->env = env;
        memcpy((void *)param, _param, sizeof(recorder_param_t));
        scene->param = param;
        scene->op = &recoder_operator;
    }
    
    return scene;
}

static void init(audio_scene_t *scene)
{
    recorder_env_t *env = scene->env;
    recorder_param_t *param = scene->param;
    
    recoder_scene = scene;

    env->buffer = pvPortMalloc(STORE_MIC_PCM_DATA_SAMPLES * sizeof(uint16_t) * param->channels);
    env->hw = audio_hw_create(param->hw_type, NULL, param->base_addr, AUDIO_HW_DIR_IN, param->sample_rate, param->channels);
    env->hw_output = audio_hw_output_add(env->hw, hw_receive_pcm);
    /* create encoder */
    env->encoder = audio_encoder_init(param->encoder_type, param->channels, param->sample_rate, FRAME_MAX_LENGTH_FIT_SINGLE, &param->encoder_param);
}

static void destroy(audio_scene_t *scene)
{
    recorder_env_t *env = scene->env;
    
    audio_hw_destroy(env->hw);
    audio_encoder_destroy(env->encoder);
    
    vPortFree(env->buffer);
    vPortFree(scene->env);
    vPortFree(scene->param);
    vPortFree(scene);
    recoder_scene = NULL;
}

static void event_handler(audio_scene_t *scene, audio_scene_evt_t *evt)
{
    recorder_env_t *env = scene->env;
    recorder_param_t *param = scene->param;

    switch(evt->type) {
        case AUDIO_SCENE_EVT_TYPE_HW_IN_NEW_SAMPLES:
            {
                int encoded_frame_count;
                audio_scene_evt_hw_in_new_samples_t *_evt = (void *)evt;
                uint32_t adc_new_samples = _evt->adc_new_samples;
                while(adc_new_samples) {
                    uint32_t samples = adc_new_samples > STORE_MIC_PCM_DATA_SAMPLES ? STORE_MIC_PCM_DATA_SAMPLES : adc_new_samples;
                    samples = audio_hw_read_pcm(env->hw_output, env->buffer, samples, param->channels);
                    audio_encoder_encode(env->encoder, (void *)env->buffer, samples*sizeof(uint16_t)*param->channels, param->channels, param->sample_rate);
                    adc_new_samples -= samples;
                }
                encoded_frame_count = audio_encoder_get_frame_count(env->encoder);
                while(encoded_frame_count--) {
                    audio_encoder_frame_t *frame;
                    frame = audio_encoder_frame_pop(env->encoder);
                    if (param->report_cb) {
                        param->report_cb(param->report_param, frame->data, frame->length);
                    }
                    audio_encoder_frame_release(frame);
                }
            }
            break;
        default:
            break;
    }
}

audio_scene_operator_t recoder_operator = {
    .allocate = allocate,
    .init = init,
    .destroy = destroy,
    .event_handler = event_handler,
    .decoder_started = NULL,
    .support_tone = false,
};
