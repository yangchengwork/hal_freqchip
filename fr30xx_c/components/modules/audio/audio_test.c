#include "audio_test.h"
#include "audio_scene.h"

#include "fr30xx.h"

#include "mp3_sample.h"
#include "sbc_sample.h"
#include "msbc_sample.h"

static audio_scene_t *local_playback_scene;
static audio_scene_t *a2dp_sink_scene;
static audio_scene_t *a2dp_source_scene;
static audio_scene_t *sco_source_scene;
static audio_scene_t *voice_recognize_scene;

static uint32_t a2dp_sink_sbc_index = 0;
static bool msbc_ready = false;

static void local_playback_request_raw_data(audio_decoder_t *decoder)
{
#define RAW_FRAME_SIZE          512
    static uint32_t index = 0;
    uint32_t length;
    uint32_t total_length = mp3_sample_get_size();
    audio_ret_t ret;

    {
        fputc('{', &__stdout);
        do {
            length = RAW_FRAME_SIZE;
            if ((length + index) > total_length) {
                length = total_length - index;
            }

            ret = audio_scene_decode(decoder, &mp3_sample[index], &length);
            index += length;
            if (index >= total_length) {
                index = 0;
                break;
            }
        } while (ret != AUDIO_RET_OUTPUT_ALMOTE_FULL);
        fputc('}', &__stdout);
    }
}

static void a2dp_sink_request_raw_data(audio_decoder_t *decoder)
{
#define RAW_FRAME_SIZE          512
    
    uint32_t length;
    uint32_t total_length = sbc_sample_get_size();
    audio_ret_t ret;

    {
        fputc('{', &__stdout);
        do {
            length = RAW_FRAME_SIZE;
            if ((length + a2dp_sink_sbc_index) > total_length) {
                length = total_length - a2dp_sink_sbc_index;
            }

            ret = audio_scene_decode(decoder, &sbc_sample[a2dp_sink_sbc_index], &length);
            a2dp_sink_sbc_index += length;
            if (a2dp_sink_sbc_index >= total_length) {
                a2dp_sink_sbc_index = 0;
                break;
            }
        } while (ret != AUDIO_RET_OUTPUT_ALMOTE_FULL);
        fputc('}', &__stdout);
    }
}

//static void a2dp_source_request_raw_data(audio_decoder_t *decoder)
//{
//#define RAW_FRAME_SIZE          512
//    static uint32_t index = 0;
//    uint32_t length;
//    uint32_t total_length = mp3_sample_get_size();
//    audio_ret_t ret;

//    {
//        fputc('{', &__stdout);
//        do {
//            length = RAW_FRAME_SIZE;
//            if ((length + index) > total_length) {
//                length = total_length - index;
//            }

//            ret = audio_scene_decode(decoder, &mp3_sample[index], &length);
//            index += length;
//            if (index >= total_length) {
//                index = 0;
//                break;
//            }
//        } while (ret != AUDIO_RET_OUTPUT_ALMOTE_FULL);
//        fputc('}', &__stdout);
//    }
//}

//static void sco_request_raw_data(audio_decoder_t *decoder)
//{
//#ifdef RAW_FRAME_SIZE
//#undef RAW_FRAME_SIZE
//#endif
//#define RAW_FRAME_SIZE          57
//    static uint32_t index = 0;
//    uint32_t length;
//    uint32_t total_length = msbc_sample_get_size();
//    audio_ret_t ret;
//    
//    if (msbc_ready == false) {
//        return;
//    }
//    msbc_ready = false;

//    {
//        fputc('{', &__stdout);
//        do {
//            length = RAW_FRAME_SIZE;
//            if ((length + index) > total_length) {
//                length = total_length - index;
//            }

//            ret = audio_scene_decode(decoder, &msbc_sample[index], &length);
//            index += length;
//            if (index >= total_length) {
//                index = 0;
//                break;
//            }
//        } while (0);
//        fputc('}', &__stdout);
//    }
//}

//static void tone_request_raw_data(audio_decoder_t *decoder)
//{
//#ifdef RAW_FRAME_SIZE
//#undef RAW_FRAME_SIZE
//#endif
//#define RAW_FRAME_SIZE          512
//    static uint32_t index = 0;
//    uint32_t length;
//    uint32_t total_length = mp3_sample_get_size();
//    audio_ret_t ret;

//    {
//        fputc('[', &__stdout);
//        do {
//            length = RAW_FRAME_SIZE;
//            if ((length + index) > total_length) {
//                length = total_length - index;
//            }

//            ret = audio_scene_decode(decoder, &mp3_sample[index], &length);
//            index += length;
//            if (index >= total_length) {
//                index = 0;
//                audio_scene_tone_stop();
//                break;
//            }
//        } while (ret != AUDIO_RET_OUTPUT_ALMOTE_FULL);
//        fputc(']', &__stdout);
//    }
//}

static void local_playback_start(void)
{
    audio_scene_param_local_playback_t param;
    
    param.sample_rate = 44100;
    param.channels = 2;
    param.hw_type = AUDIO_HW_TYPE_CODEC;
    param.hw_base_addr = I2S0_BASE;
    param.req_dec_cb = local_playback_request_raw_data;
    local_playback_scene = audio_scene_create(AUDIO_SCENE_TYPE_LOCAL_PLAYBACK, &param);
}

static void local_playback_stop(void)
{
    audio_scene_destroy(local_playback_scene);
}

static void a2dp_sink_start(void)
{
    audio_scene_param_a2dp_sink_t param;
    
    param.sample_rate = 44100;
    param.channels = 2;
    param.audio_type = AUDIO_TYPE_SBC;
    param.hw_type = AUDIO_HW_TYPE_CODEC;
    param.hw_base_addr = I2S0_BASE;
    param.req_dec_cb = a2dp_sink_request_raw_data;
    a2dp_sink_scene = audio_scene_create(AUDIO_SCENE_TYPE_A2DP_SINK, &param);
}

static void a2dp_sink_stop(void)
{
    audio_scene_destroy(a2dp_sink_scene);
}

//static void a2dp_source_start(void)
//{
//    audio_scene_param_a2dp_source_t param;
//    
//    param.sample_rate = 44100;
//    param.channels = 2;
//    param.req_dec_cb = a2dp_source_request_raw_data;
//    param.audio_type = AUDIO_TYPE_SBC;
//    a2dp_source_scene = audio_scene_create(AUDIO_SCENE_TYPE_A2DP_SOURCE, &param);
//}

//static void a2dp_source_stop(void)
//{
//    audio_scene_destroy(a2dp_source_scene);
//}

//static void a2dp_source_encode(void)
//{
//    audio_scene_encode_reqeust(40);
//}

static void sco_start(void)
{
    audio_scene_param_sco_t param;
    
    if (sco_source_scene) {
        return;
    }
    
    param.audio_type = AUDIO_TYPE_MSBC;
    param.hw_type = AUDIO_HW_TYPE_CODEC;
    param.hw_base_addr = I2S0_BASE;
    param.report_enc_cb = NULL;
    param.sample_rate = 16000;
    
    sco_source_scene = audio_scene_create(AUDIO_SCENE_TYPE_SCO, &param);
    
    __SYSTEM_TIMER0_CLK_ENABLE();
    timer_init(Timer0, (system_get_CoreClock() / 4000) * 30); // 7.5ms
    timer_start(Timer0);
    NVIC_EnableIRQ(TIMER0_IRQn);
}

static void sco_stop(void)
{
    NVIC_DisableIRQ(TIMER0_IRQn);
    timer_stop(Timer0);
    __SYSTEM_TIMER0_CLK_DISABLE();
    audio_scene_destroy(sco_source_scene);
    sco_source_scene = NULL;
}

//static void tone_start(void)
//{
//    audio_scene_param_tone_t param;

//    param.audio_type = AUDIO_TYPE_MP3;
//    param.hw_type = AUDIO_HW_TYPE_I2S;
//    param.hw_base_addr = I2S0_BASE;
//    param.req_dec_cb = tone_request_raw_data;
//    
//    audio_scene_tone_play(&param, 1024, configMAX_PRIORITIES-1);
//}

static void sco_income_data(void)
{
    static uint32_t msbc_index = 0;
    
    audio_scene_recv_raw_data(sco_source_scene, true, &msbc_sample[msbc_index], 57);
    msbc_index += 57;
    if (msbc_index >= msbc_sample_get_size()) {
        msbc_index = 0;
    }
}

//static void tone_stop(void)
//{
//    audio_scene_tone_stop();
//}

void voice_recognize_start(void)
{
    audio_scene_param_voice_recognize_t param;
    
    param.hw_type = AUDIO_HW_TYPE_CODEC;
    param.sample_rate = 16000;
    
    voice_recognize_scene = audio_scene_create(AUDIO_SCENE_TYPE_VOICE_RECOGNIZE, &param);
}

void voice_recognize_stop(void)
{
    audio_scene_destroy(voice_recognize_scene);
}

void audio_test(uint8_t sub_cmd, uint8_t *param)
{
    switch(sub_cmd) {
        case 'A':
            local_playback_start();
            break;
        case 'B':
            local_playback_stop();
            break;
//        case 'C':
//            tone_start();
//            break;
//        case 'D':
//            tone_stop();
//            break;
//        case 'E':
//            a2dp_source_start();
//            break;
//        case 'F':
//            a2dp_source_stop();
//            break;
//        case 'G':
//            a2dp_source_encode();
//            break;
        case 'H':
            a2dp_sink_sbc_index = 0;
            a2dp_sink_start();
            break;
        case 'I':
            a2dp_sink_stop();
            break;
        case 'J':
            sco_start();
            break;
        case 'K':
            sco_stop();
            break;
        case 'L':
            sco_income_data();
            break;
        case 'M':
            voice_recognize_start();
            break;
        case 'N':
            voice_recognize_stop();
            break;
        default:
            break;
    }

    printf("OK\r\n");
}
