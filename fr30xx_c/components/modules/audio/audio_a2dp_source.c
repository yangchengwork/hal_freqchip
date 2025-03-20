#include "audio_scene.h"
#include "audio_hw.h"
#include "audio_decoder.h"
#include "dsp_mem.h"

#include "audio_a2dp_source.h"

#define A2DP_SOURCE_PACKET_SAMPLE_CNT           640     //unit: sample, 640 sample ---> 5 sbc frame, about (5*128/44100) seconds 

#define RAW_DATA_BUFFER_SIZE                    128

enum {
    A2DP_SOURCE_EVT_REQ_NEW_SBC_PACKET = AUDIO_SCENE_EVT_YTPE_MAX,
};

typedef struct {
    audio_decoder_t *decoder;
    audio_decoder_output_t *decoder_to_encoder;
    audio_encoder_t *encoder;
    audio_hw_t *audio_hw;
    audio_hw_output_t *audio_hw_output;         // used to receive ADC data

    uint32_t i2s_cnt;
    
    uint8_t *raw_data;                          // used to store raw encoded data
} a2dp_source_env_t;

static audio_scene_t *a2dp_source_scene = NULL;

static void hw_receive_output_pcm(uint32_t samples)
{
    a2dp_source_env_t *env = a2dp_source_scene->env;
    audio_scene_evt_t *evt;
    
    env->i2s_cnt ++;
    if(env->i2s_cnt == 40){
        env->i2s_cnt = 0;
        evt = (void *)pvPortMalloc(sizeof(audio_scene_evt_t));
        if (evt) {
            evt->type = A2DP_SOURCE_EVT_REQ_NEW_SBC_PACKET;
            evt->scene = a2dp_source_scene;
            audio_scene_send_event(evt);
        }        
    }
}

static void decoder_request_raw_data_handler(audio_decoder_t *decoder, uint8_t event)
{
    if ( event == AUDIO_DECODER_EVENT_REQ_RAW_DATA )
    {
        audio_scene_evt_req_encoded_frame_t *evt = (void *)pvPortMalloc( sizeof(audio_scene_evt_req_encoded_frame_t) );

        if ( evt )
        {
            evt->evt.type = AUDIO_SCENE_EVT_TYPE_REQ_ENCODED_FRAME;
            evt->evt.scene = a2dp_source_scene;
            evt->decoder = decoder;
            audio_scene_send_event( &evt->evt );
        }
    }
}

static audio_scene_t *allocate(void *_param)
{
    audio_scene_t *scene = pvPortMalloc(sizeof(audio_scene_t));

    if (scene) {
        /* init a2dp sink enviroment */
        a2dp_source_env_t *env = pvPortMalloc(sizeof(a2dp_source_env_t));
        if (env == NULL) {
            vPortFree(scene);
            return NULL;
        }
        
        /* allocate buffer to save a2dp sink parameters */
        audio_a2dp_source_param_t *param = pvPortMalloc(sizeof(audio_a2dp_source_param_t));
        if (param == NULL) {
            vPortFree(env);
            vPortFree(scene);
            return NULL;
        }
        
        scene->env = env;
        memcpy((void *)param, _param, sizeof(audio_a2dp_source_param_t));
        scene->param = param;
        scene->op = &audio_a2dp_source_operator;
    }
    
    return scene;
}

static void init(audio_scene_t *scene)
{
    audio_a2dp_source_param_t *param = scene->param;
    a2dp_source_env_t *env = scene->env;
    uint16_t frame_len;
    
    a2dp_source_scene = scene;
    
    env->i2s_cnt = 0;
    env->raw_data = pvPortMalloc(RAW_DATA_BUFFER_SIZE);

    /* initialize audio decoder module */
    audio_decoder_init(param->channels, param->sample_rate);
    
    /* create and start a decoder */
    env->decoder = audio_decoder_add(param->audio_input_type, &param->decoder_param, decoder_request_raw_data_handler);
    audio_decoder_start(env->decoder);
    
    //audio_scene_env.env.a2dp_source.decoder_to_hw = audio_decoder_output_add(true, AUDIO_CHANNELS_STEREO);
    env->decoder_to_encoder = audio_decoder_output_add(true, AUDIO_CHANNELS_STEREO);
    
    if ((param->audio_output_type == AUDIO_TYPE_SBC)
            || (param->audio_output_type == AUDIO_TYPE_SBC_V2)) {
        /* create encoder */
        struct sbc_encoder_param *_param = &param->encoder_param.sbc;

//        _param.i_samp_freq = param->sbc_param.sample_rate;
//        _param.i_bitpool = param->sbc_param.bitpool;
//        _param.i_blocks = 16;//param->sbc_param.num_blocks;
//        _param.i_num_chan = 2;//param->sbc_param.num_channels;
//        _param.i_subbands = 8;//param->sbc_param.num_subbands;
//        _param.i_snr = param->sbc_param.alloc_method;
        
        /* calc single sbc frame length, max frame len is 119 */
        uint16_t temp = (1*_param->i_subbands) + (_param->i_blocks*_param->i_bitpool);
        frame_len = 4 + ((4*_param->i_subbands*_param->i_num_chan)>>3) + (temp>>3);
        if(temp%8){
            frame_len++;
        }
    }
    else {
        frame_len = 128;
    }
    
    /* max frame len set to 5*frame_len, shall complied with hw i2s trigger*/
    env->encoder = audio_encoder_init(param->audio_output_type, param->channels, param->sample_rate, 5*frame_len, &param->encoder_param);
    
    /* initialize audio hardware */
    env->audio_hw = audio_hw_create(param->hw_type, 
                                    NULL,//hw_request_a2dp_source_pcm_cb, 
                                    param->hw_base_addr, 
                                    AUDIO_HW_DIR_IN, 
                                    param->sample_rate,
                                    AUDIO_CHANNELS_STEREO);
    env->audio_hw_output = audio_hw_output_add(env->audio_hw, hw_receive_output_pcm);
}

static void destroy(audio_scene_t *scene)
{
    a2dp_source_env_t *env = scene->env;
    
    /* release audio hardware */
    audio_hw_destroy(env->audio_hw);

    /* release audio decoder module */    
    audio_decoder_destroy();
            
    /* release audio encoder module */   
    audio_encoder_destroy(env->encoder);
    
    vPortFree(env->raw_data);
    vPortFree(scene->env);
    vPortFree(scene->param);
    vPortFree(scene);
    a2dp_source_scene = NULL;
}

static void event_handler(audio_scene_t *scene, audio_scene_evt_t *evt)
{
    a2dp_source_env_t *env = scene->env;
    audio_a2dp_source_param_t *param = scene->param;

//    printf("a2dp source evt hanlder %d\r\n",evt->type);
    switch (evt->type) {
        case AUDIO_SCENE_EVT_TYPE_REQ_ENCODED_FRAME:
            {
                uint32_t length = 0;
                int ret = AUDIO_RET_NEED_MORE;
                audio_scene_evt_req_encoded_frame_t *_evt = (void *)evt;
                                                           
                if (_evt->decoder == env->decoder) {
                    if (audio_decoder_decode(env->decoder,
                                                NULL, &length) != AUDIO_RET_OUTPUT_ALMOTE_FULL) {       
                        if(param->dec_req_raw_cb){
                            do {
                                length = param->dec_req_raw_cb(env->raw_data, RAW_DATA_BUFFER_SIZE);
                                if (length) {
                                    ret = audio_decoder_decode(env->decoder, env->raw_data, &length);
                                }
                                else {
                                    break;
                                }
                            } while (ret == AUDIO_RET_NEED_MORE);
                        }
                    }
                }                
            }
            break;
        case A2DP_SOURCE_EVT_REQ_NEW_SBC_PACKET:
            {
                int16_t *pcm;
                audio_ret_t ret;
                uint32_t encoded_frame;

                pcm = (int16_t *)pvPortMalloc(A2DP_SOURCE_PACKET_SAMPLE_CNT * sizeof(uint16_t) * param->channels);
                audio_decoder_get_pcm(env->decoder_to_encoder, pcm, A2DP_SOURCE_PACKET_SAMPLE_CNT, param->channels);

                if(audio_decoder_is_started(env->decoder)) {
                    ret = audio_encoder_encode(env->encoder, (const uint8_t *)pcm, A2DP_SOURCE_PACKET_SAMPLE_CNT * sizeof(uint16_t) * param->channels,  \
                                                param->channels, param->sample_rate);
                    /* send encoded frame to peer device */
                    encoded_frame = audio_encoder_get_frame_count(env->encoder);
                    while (encoded_frame--) {
                        audio_encoder_frame_t *frame;
                        frame = audio_encoder_frame_pop(env->encoder);
                        if (param->report_enc_cb) {
                            param->report_enc_cb(param->report_enc_arg, frame->data, frame->length);
                        }
                        audio_encoder_frame_release(frame);
                    }
                }
                vPortFree((void *)pcm);
            }
            break;
        default:
            while(1);
            break;
    }
}

audio_scene_operator_t audio_a2dp_source_operator = {
    .allocate = allocate,
    .init = init,
    .destroy = destroy,
    .event_handler = event_handler,
    .decoder_started = NULL,
    .support_tone = false,
};
