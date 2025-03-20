#include <assert.h>

#include "audio_decoder.h"
#include "audio_hw.h"

#include "local_playback.h"

#define TONE_RAW_DATA_BUFFER_SIZE           128

typedef struct {
    audio_hw_t *audio_hw;
    audio_decoder_t *decoder;
    audio_decoder_output_t *decoder_to_hw;
    
    uint8_t *raw_data;
} local_playback_env_t;

static audio_scene_t *local_playback_scene = NULL;

static void hw_request_local_playback_pcm_cb(void *pcm, uint32_t samples, uint8_t channels)
{
    if (local_playback_scene == NULL) {
        return;
    }
    local_playback_env_t *_env = local_playback_scene->env;

    /* request PCM data from audio decoder */
    audio_decoder_get_pcm(_env->decoder_to_hw, pcm, samples, channels);
}

/* this function may be called from interrupt, just send message to audio_scene_task */
static void decoder_request_raw_data_cb(audio_decoder_t *decoder, uint8_t event)
{
    if ( event == AUDIO_DECODER_EVENT_REQ_RAW_DATA )
    {
        audio_scene_evt_req_encoded_frame_t *evt = (void *)pvPortMalloc( sizeof(audio_scene_evt_req_encoded_frame_t) );

        if ( evt )
        {
            evt->evt.type = AUDIO_SCENE_EVT_TYPE_REQ_ENCODED_FRAME;
            evt->evt.scene = local_playback_scene;
            evt->decoder = decoder;
            audio_scene_send_event( &evt->evt );
        }
    }
}

static audio_scene_t *allocate(void *_param)
{
    audio_scene_t *scene = pvPortMalloc(sizeof(audio_scene_t));

    if (scene) {
        local_playback_env_t *env = pvPortMalloc(sizeof(local_playback_env_t));
        if (env == NULL) {
            vPortFree(scene);
            return NULL;
        }
        
        local_playback_param_t *param = pvPortMalloc(sizeof(local_playback_param_t));
        if (param == NULL) {
            vPortFree(env);
            vPortFree(scene);
            return NULL;
        }
        
        scene->env = env;
        memcpy((void *)param, _param, sizeof(local_playback_param_t));
        scene->param = param;
        scene->op = &audio_local_playback_operator;
    }
    
    return scene;
}

static void init(audio_scene_t *scene)
{
    local_playback_scene = scene;
    
    local_playback_env_t *_env = scene->env;
    local_playback_param_t *_param = scene->param;
    
    _env->raw_data = pvPortMalloc(TONE_RAW_DATA_BUFFER_SIZE);
    
    audio_decoder_init(_param->channels, _param->sample_rate);
    _env->decoder = audio_decoder_add(_param->audio_type, &_param->decoder_param, decoder_request_raw_data_cb);
    assert(_env->decoder != NULL);
    audio_decoder_start(_env->decoder);
    
    /* add an output to initialized audio decoder module */
    _env->decoder_to_hw = audio_decoder_output_add(true, AUDIO_CHANNELS_STEREO);

    /* initialize audio hardware */
    _env->audio_hw = audio_hw_create(_param->hw_type, 
                                            hw_request_local_playback_pcm_cb, 
                                            _param->hw_base_addr, 
                                            AUDIO_HW_DIR_OUT, 
                                            _param->sample_rate,
                                            _param->channels);
}

static void destroy(audio_scene_t *scene)
{
    if (scene == NULL) {
        return;
    }

    local_playback_env_t *env = scene->env;   
    
    /* release audio hardware */
    audio_hw_destroy(env->audio_hw);
    
    /* release audio decoder module */    
    audio_decoder_destroy();
    
    vPortFree(env->raw_data);
    vPortFree(scene->env);
    vPortFree(scene->param);
    vPortFree(scene);

    local_playback_scene = NULL;
}

static void event_handler(audio_scene_t *scene, audio_scene_evt_t *evt)
{
    local_playback_env_t *env = scene->env;
    local_playback_param_t *param = scene->param;

    switch(evt->type) {
        case AUDIO_SCENE_EVT_TYPE_REQ_ENCODED_FRAME:
            {
                uint32_t length = 0;
                audio_scene_evt_req_encoded_frame_t *_evt = (void *)evt;
                
                if (env->decoder == _evt->decoder) {
                    if (audio_decoder_decode(env->decoder,
                                                NULL, &length) != AUDIO_RET_OUTPUT_ALMOTE_FULL) {
                        if(param->req_raw_cb){
                            int ret;
                            do {
                                length = param->req_raw_cb(env->raw_data, TONE_RAW_DATA_BUFFER_SIZE);
                                if (length) {
                                    ret = audio_decoder_decode(env->decoder, env->raw_data, &length);
                                }
                                else {
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

audio_scene_operator_t audio_local_playback_operator = {
    .allocate = allocate,
    .init = init,
    .destroy = destroy,
    .event_handler = event_handler,
    .decoder_started = NULL,
    .support_tone = false,
};
