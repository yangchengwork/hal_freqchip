#include "audio_scene.h"
#include "audio_hw.h"
#include "audio_decoder.h"
#include "dsp_mem.h"
#include "algorithm.h"

#include "audio_sco.h"

#define AUDIO_ALGO_ADC_IN_BUFFER_FRAME_COUNT    8       // unit: algo_frame_size

#define DECODER_DATA_READY_THD                  3
#define DECODER_DATA_MAX_THD                    10

#define TONE_RAW_DATA_BUFFER_SIZE               128

typedef struct {
    audio_decoder_t *decoder;       // for sco data decoder, PLC is included
    audio_decoder_output_t *decoder_to_algo;    // decoded data routed to algorithm
    audio_decoder_output_t *decoder_to_hw;      // decoded data routed to DAC
    audio_encoder_t *encoder;       // for sco data encoder, input data is output of AEC
    audio_hw_t *audio_hw;
    audio_hw_output_t *audio_hw_output;         // used to receive ADC data
    void *audio_algo_handle;
    
    struct co_list sco_data_list;
    uint16_t sco_data_counter;
    uint8_t start_thd;
    
    uint32_t algo_frame_size;       // unit is sample

    int16_t *decoder_output;        // next step is algorithm, size is algo_frame_size
    uint32_t decoder_output_wr_ptr; // unit is sample

    int16_t *adc_input;             // next step is algorithm, size is adc_input_size = N * algo_frame_size
    uint32_t adc_input_size;        // unit is sample
    uint32_t adc_input_wr_ptr;      // unit is sample
    uint32_t adc_input_rd_ptr;      // unit is sample
} audio_sco_env_t;

static audio_scene_t *sco_scene = NULL;

/* this function may be called from interrupt, just send message to audio_scene_task */
static void decoder_request_raw_data_cb(audio_decoder_t *decoder, uint8_t event)
{
    if ( event == AUDIO_DECODER_EVENT_REQ_RAW_DATA )
    {
        audio_scene_evt_req_encoded_frame_t *evt = (void *)pvPortMalloc( sizeof(audio_scene_evt_req_encoded_frame_t) );

        if ( evt )
        {
            evt->evt.type = AUDIO_SCENE_EVT_TYPE_REQ_ENCODED_FRAME;
            evt->evt.scene = sco_scene;
            evt->decoder = decoder;
            audio_scene_send_event( &evt->evt );
        }
    }
}

static void hw_request_sco_pcm_cb(void *pcm, uint32_t samples, uint8_t channels)
{
    audio_sco_env_t *env = sco_scene->env;
    
    /* request PCM data by speaker, these data are output of Audio algorithm */
    audio_decoder_get_pcm(env->decoder_to_hw, pcm, samples, channels);
}

static void hw_receive_adc_pcm_cb(uint32_t samples)
{
    audio_scene_evt_hw_in_new_samples_t *evt = (void *)pvPortMalloc(sizeof(audio_scene_evt_hw_in_new_samples_t));
    
    if (evt) {
        evt->evt.type = AUDIO_SCENE_EVT_TYPE_HW_IN_NEW_SAMPLES;
        evt->evt.scene = sco_scene;
        evt->adc_new_samples = samples;
        audio_scene_send_event(&evt->evt);
    }
}

static audio_scene_t *allocate(void *_param)
{
    audio_scene_t *scene = pvPortMalloc(sizeof(audio_scene_t));

    if (scene) {
        /* init a2dp sink enviroment */
        audio_sco_env_t *env = pvPortMalloc(sizeof(audio_sco_env_t));
        if (env == NULL) {
            vPortFree(scene);
            return NULL;
        }
        
        /* allocate buffer to save a2dp sink parameters */
        audio_sco_param_t *param = pvPortMalloc(sizeof(audio_sco_param_t));
        if (param == NULL) {
            vPortFree(env);
            vPortFree(scene);
            return NULL;
        }
        
        scene->env = env;
        memcpy((void *)param, _param, sizeof(audio_sco_param_t));
        scene->param = param;
        scene->op = &audio_sco_operator;
    }
    
    return scene;
}

static void init(audio_scene_t *scene)
{
    audio_sco_param_t *param = scene->param;
    audio_sco_env_t *env = scene->env;
    
    sco_scene = scene;
    
    co_list_init(&env->sco_data_list);
    env->sco_data_counter = 0;
    env->start_thd = DECODER_DATA_READY_THD;
    
    /* create audio algorithm instance */
    /*
        enum
        {
            kAgcModeUnchanged,
            kAgcModeAdaptiveAnalog,
            kAgcModeAdaptiveDigital,
            kAgcModeFixedDigital
        };
     */
    env->audio_algo_handle = algorithm_init(/*AUDIO_ALGO_SEL_AGC | AUDIO_ALGO_SEL_NS | */AUDIO_ALGO_SEL_AEC,
                                            param->sample_rate, 
                                            3, 3,
                                            &env->algo_frame_size);
    
    /* request buffers */
    env->decoder_output = dsp_mem_alloc(env->algo_frame_size * sizeof(int16_t));
    env->decoder_output_wr_ptr = 0;
    env->adc_input = dsp_mem_alloc(env->algo_frame_size * sizeof(int16_t) * AUDIO_ALGO_ADC_IN_BUFFER_FRAME_COUNT);
    env->adc_input_size = env->algo_frame_size * AUDIO_ALGO_ADC_IN_BUFFER_FRAME_COUNT;
    env->adc_input_wr_ptr = 0;
    env->adc_input_rd_ptr = 0;

    /* initialize audio decoder module */
    audio_decoder_init(AUDIO_CHANNELS_MONO, param->sample_rate);
    
    /* create and start a decoder */
    env->decoder = audio_decoder_add(param->audio_type, &param->decoder_param, decoder_request_raw_data_cb);

    /* add an output to initialized audio decoder module */
    env->decoder_to_algo = audio_decoder_output_add(true, AUDIO_CHANNELS_MONO);
    /* add an output to initialized audio decoder module */
    env->decoder_to_hw = audio_decoder_output_add(true, AUDIO_CHANNELS_MONO);
    
    /* create encoder */
    if (param->audio_type == AUDIO_TYPE_MSBC) {
        env->encoder = audio_encoder_init(param->audio_type, AUDIO_CHANNELS_MONO, param->sample_rate, 57, &param->encoder_param);
    }
    else {
        env->encoder = audio_encoder_init(param->audio_type, AUDIO_CHANNELS_MONO, param->sample_rate, 120, &param->encoder_param);
    }

    /* initialize audio hardware */
    env->audio_hw = audio_hw_create(param->hw_type, 
                                    hw_request_sco_pcm_cb, 
                                    param->hw_base_addr, 
                                    AUDIO_HW_DIR_INOUT, 
                                    param->sample_rate,
                                    AUDIO_CHANNELS_STEREO);
    env->audio_hw_output = audio_hw_output_add(env->audio_hw, hw_receive_adc_pcm_cb);
}

static void destroy(audio_scene_t *scene)
{
    audio_sco_env_t *env = scene->env;

    /* release audio hardware */
    audio_hw_destroy(env->audio_hw);

    /* release audio encoder */
    audio_encoder_destroy(env->encoder);

    /* release audio decoder module */    
    audio_decoder_destroy();

    /* release audio algorithm instance */
    algorithm_destroy(env->audio_algo_handle);

    /* free allocated buffer */
    dsp_mem_free(env->decoder_output);
    dsp_mem_free(env->adc_input);

    audio_data_element_t *elt = (void *)co_list_pop_front(&env->sco_data_list);
    while (elt) {
        vPortFree(elt);
        elt = (void *)co_list_pop_front(&env->sco_data_list);
    }
    
    vPortFree(scene->env);
    vPortFree(scene->param);
    vPortFree(scene);
    sco_scene = NULL;
}

static void event_handler(audio_scene_t *scene, audio_scene_evt_t *evt)
{
    audio_sco_env_t *sco_env = scene->env;
    audio_sco_param_t *param = scene->param;

    switch (evt->type) {
        case AUDIO_SCENE_EVT_TYPE_REQ_ENCODED_FRAME:
            {
                uint32_t length = 0;
                audio_scene_evt_req_encoded_frame_t *_evt = (void *)evt;
                if (_evt->decoder == sco_env->decoder) {
                    if (audio_decoder_decode(sco_env->decoder,
                                                NULL, &length) != AUDIO_RET_OUTPUT_ALMOTE_FULL) {
                        if (sco_env->start_thd == 0) {
                            audio_data_element_t *elt;
                            int ret;
                            elt = (void *)co_list_pick(&sco_env->sco_data_list);
                            while (elt) {
                                uint32_t length = elt->length - elt->offset;
                                if (elt->valid) {
                                    uint32_t length = elt->length - elt->offset;
                                    ret = audio_decoder_decode(sco_env->decoder, &elt->buffer[elt->offset], &length);
                                    elt->offset += length;
                                }
                                else {
                                    uint32_t length = AUDIO_SPECIAL_LENGTH_FOR_PLC;
                                    ret = audio_decoder_decode(sco_env->decoder, NULL, &length);
                                    elt->offset += length;
                                }
                                if (elt->length == elt->offset) {
                                    elt = (void *)co_list_pop_front(&sco_env->sco_data_list);
                                    sco_env->sco_data_counter--;
                                    vPortFree(elt);
                                    elt = (void *)co_list_pick(&sco_env->sco_data_list);
                                }
                                if (ret == AUDIO_RET_OUTPUT_ALMOTE_FULL) {
                                    break;
                                }
                            }
                        }
                    }
                }
//                else if (_evt->decoder == sco_env->decoder_tone) {
//                    if (audio_decoder_decode(sco_env->decoder_tone,
//                                                NULL, &length) != AUDIO_RET_OUTPUT_ALMOTE_FULL) {
//                        if(sco_env->tone_req_raw_cb){
//                            int ret;
//                            do {
//                                length = sco_env->tone_req_raw_cb(sco_env->tone_raw_data, TONE_RAW_DATA_BUFFER_SIZE);
//                                if (length) {
//                                    ret = audio_decoder_decode(sco_env->decoder_tone, sco_env->tone_raw_data, &length);
//                                }
//                                else {
//                                    break;
//                                }
//                            } while (ret == AUDIO_RET_NEED_MORE);
//                        }
//                    }
//                }
            }
            break;
        case AUDIO_SCENE_EVT_TYPE_RECV_ENCODED_FRAME:
            {
                audio_scene_evt_recv_encoded_data_t *_evt = (void *)evt;
                if ((_evt->raw_frame->valid) || (sco_env->start_thd == 0)) {
                    co_list_push_back(&sco_env->sco_data_list, &_evt->raw_frame->hdr);
                    sco_env->sco_data_counter++;
                    if (sco_env->start_thd) {
                        sco_env->start_thd--;
                        if (sco_env->start_thd == 0) {
                            audio_decoder_start(sco_env->decoder);
                        }
                    }
                }
                else {
                    vPortFree(_evt->raw_frame);
                }
            }
            break;
        case AUDIO_SCENE_EVT_TYPE_HW_IN_NEW_SAMPLES:
            {
                uint32_t sco_in_length;
                uint32_t left_space;
                audio_scene_evt_hw_in_new_samples_t *_evt = (void *)evt;
                uint32_t adc_samples = _evt->adc_new_samples;
                uint32_t available_samples;
                
                /* save adc data into framebuffer */
                left_space = sco_env->adc_input_size - sco_env->adc_input_wr_ptr;
                if (left_space > adc_samples) {
                    audio_hw_read_pcm(sco_env->audio_hw_output, &sco_env->adc_input[sco_env->adc_input_wr_ptr], adc_samples, AUDIO_CHANNELS_MONO);
                    sco_env->adc_input_wr_ptr += adc_samples;
                }
                else {
                    audio_hw_read_pcm(sco_env->audio_hw_output, &sco_env->adc_input[sco_env->adc_input_wr_ptr], left_space, AUDIO_CHANNELS_MONO);
                    adc_samples -= left_space;
                    sco_env->adc_input_wr_ptr = 0;
                    if (adc_samples) {
                        audio_hw_read_pcm(sco_env->audio_hw_output, &sco_env->adc_input[0], adc_samples, AUDIO_CHANNELS_MONO);
                        sco_env->adc_input_wr_ptr = adc_samples;
                    }
                }
                
                /* check whether received ADC data are enough for audio algorithm */
                if (sco_env->adc_input_wr_ptr >= sco_env->adc_input_rd_ptr) {
                    available_samples = sco_env->adc_input_wr_ptr - sco_env->adc_input_rd_ptr;
                }
                else {
                    available_samples = sco_env->adc_input_size + sco_env->adc_input_wr_ptr - sco_env->adc_input_rd_ptr;
                }
                
                if (sco_env->decoder_output_wr_ptr < sco_env->algo_frame_size) {
                    /* request decoded PCM data (from ESCO IN), and send data to audio algorithim */
                    sco_in_length = audio_decoder_get_pcm(sco_env->decoder_to_algo, 
                                                            &sco_env->decoder_output[sco_env->decoder_output_wr_ptr],
                                                            sco_env->algo_frame_size - sco_env->decoder_output_wr_ptr, 
                                                            AUDIO_CHANNELS_MONO);
                    sco_env->decoder_output_wr_ptr += sco_in_length;
                }
                
                if (available_samples >= sco_env->algo_frame_size) {
                    if (sco_env->decoder_output_wr_ptr >= sco_env->algo_frame_size) {
                        int16_t *output = NULL;
                        uint32_t encoded_frame;
                        uint32_t aec_out_samples = sco_env->algo_frame_size;

                        sco_env->decoder_output_wr_ptr = 0;
                        
                        /* request start algorithim */
                        algorithm_launch(sco_env->audio_algo_handle, 
                                            &sco_env->decoder_output[0], 
                                            &sco_env->adc_input[sco_env->adc_input_rd_ptr], 
                                            &output);
                        sco_env->adc_input_rd_ptr += sco_env->algo_frame_size;
                        if (sco_env->adc_input_rd_ptr >= sco_env->adc_input_size) {
                            sco_env->adc_input_rd_ptr = 0;
                        }

                        /* use AEC output to do encoder, send encoded SCO frame to remote device */
                        uint32_t sub_length;
                        while (aec_out_samples) {
                            if(sco_env->encoder->type == AUDIO_TYPE_PCM){
                                sub_length = (sizeof(int16_t) * aec_out_samples > sco_env->encoder->frame_max_length) ? (sco_env->encoder->frame_max_length/2) : aec_out_samples;
                            }
                            else{
                                sub_length = aec_out_samples;
                            }
                            audio_encoder_encode(sco_env->encoder, 
                                                    (const uint8_t *)output, 
                                                    sizeof(int16_t) * sub_length, 
                                                    AUDIO_CHANNELS_MONO, 
                                                    param->sample_rate);
                            aec_out_samples -= sub_length;
                            output += sub_length;                       
                        }
                        
                        /* send encoded frame to peer device */
                        encoded_frame = audio_encoder_get_frame_count(sco_env->encoder);
                        while (encoded_frame--) {
                            audio_encoder_frame_t *frame;
                            frame = audio_encoder_frame_pop(sco_env->encoder);
                            if (param->report_enc_cb) {
                                param->report_enc_cb(param->report_enc_arg, frame->data, frame->length);
                            }
                            audio_encoder_frame_release(frame);
                        }
                    }
                }
            }
            break;
//        case AUDIO_SCENE_EVT_TYPE_TONE_ADD:
//            {
//                audio_scene_evt_tone_add_t *_evt = (void *)evt;
//                if (sco_env->decoder_tone == NULL) {
//                    sco_env->tone_req_raw_cb = _evt->cb;
//                    sco_env->tone_raw_data = pvPortMalloc(TONE_RAW_DATA_BUFFER_SIZE);
//                    sco_env->decoder_tone = audio_decoder_add(_evt->audio_type, decoder_request_raw_data_cb);
//                    audio_decoder_start(sco_env->decoder_tone);
//                }
//            }
//            break;
//        case AUDIO_SCENE_EVT_TYPE_TONE_REMOVE:
//            {
//                if (sco_env->decoder_tone != NULL) {
//                    audio_decoder_remove(sco_env->decoder_tone);
//                    vPortFree(sco_env->tone_raw_data);
//                    sco_env->decoder_tone = NULL;
//                }
//            }
//            break;
        default:
            while(1);
            break;
    }
}

static bool decoder_started(audio_scene_t *scene)
{
    if ((scene == NULL) || (scene != sco_scene)) {
        return false;
    }
    else {
        audio_sco_env_t *sco_env = scene->env;
        return audio_decoder_is_started(sco_env->decoder);
    }
}

audio_scene_operator_t audio_sco_operator = {
    .allocate = allocate,
    .init = init,
    .destroy = destroy,
    .event_handler = event_handler,
    .decoder_started = decoder_started,
    .support_tone = true,
};
