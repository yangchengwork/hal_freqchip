#include "audio_decoder.h"
#include <assert.h>

#include "FreeRTOS.h"
#include "task.h"
#include "co_list.h"
#include "codec.h"

#define FIX_44100_TEMP              0
#define AUDIO_DECODER_USER_DRAM     0

#if FIX_44100_TEMP
static bool add_sample = false;
static uint32_t record_samples = 0;
#endif

#define AUDIO_DECODER_PCM_MIXED_BUFFER_DUR      120      // ms
#define AUDIO_DECODER_REQUEST_DATA_THD          60      // ms
#define AUDIO_DECODER_PCM_MIXED_BUFFER_FULL_THD 5       // ms
#define AUDIO_DECODER_PCM_RSV_AT_BEGINNING      40      // ms
#define AUDIO_DECODER_MIXED_STORE_UPPER         (audio_decoder_env.rd_ptr == 0 ? audio_decoder_env.pcm_total_samples - 1 : audio_decoder_env.rd_ptr - 1)

#if AUDIO_DECODER_PCM_RSV_AT_BEGINNING >= AUDIO_DECODER_REQUEST_DATA_THD
#error("AUDIO_DECODER_PCM_RSV_AT_BEGINNING should be smaller than AUDIO_DECODER_REQUEST_DATA_THD\r\n")
#endif

#if AUDIO_DECODER_USER_DRAM
__attribute__((section("dram_section"))) static uint8_t decoder_pcm[16*1024];
#endif

enum {
    AUDIO_DECODER_STATE_IDLE,
    AUDIO_DECODER_STATE_DECODING,
    AUDIO_DECODER_STATE_INPUT_OVER,
    AUDIO_DECODER_STATE_PCM_ALL_MIXED,
} audio_decoder_state_t;

typedef struct {
    uint8_t inited;
    uint8_t decoder_started;
    uint8_t decoder_count;

    uint8_t out_ch_num;
    uint32_t out_sample_rate;

    struct co_list decoder_list;
    struct co_list output_list;
    struct co_list decoder_wait_start_list;
    struct co_list decoder_wait_pcm_consumed_list;
    struct co_list decoder_wait_to_be_destroyed;

    /* current write pointer in Mixed pcm buffer, unit is sample */
    uint32_t wr_ptr;
    /* current read pointer from Mixed pcm buffer, unit is sample */
    uint32_t rd_ptr;

    /* used to store data after mixed */
    int16_t *pcm;
    uint32_t pcm_total_samples;

    uint32_t request_data_thd;
    uint32_t mixed_buffer_almost_full_thd;
} audio_decoder_env_t;

const static uint16_t ratio_table[] = {0, 0x8000};

static audio_decoder_env_t audio_decoder_env;
static void mix_decoded_data(audio_decoder_t *decoder);

void print_int16(uint16_t value)
{
   const static char *hex2char = "0123456789abcdef";
   fputc(hex2char[(value >> 12)&0xf], &__stdout);
   fputc(hex2char[(value >> 8)&0xf], &__stdout);
   fputc(hex2char[(value >> 4)&0xf], &__stdout);
   fputc(hex2char[(value >> 0)&0xf], &__stdout);
}

/* update write pointer after decoding a new frame or andy decoeder is removed */
static bool update_wr_ptr(void)
{
    uint32_t min_distance = 0xffffffff;
    audio_decoder_t *tmp;
    bool ret;

    /* update wr_ptr according to minimum distance */
    GLOBAL_INT_DISABLE();
    tmp = (void *)co_list_pick(&audio_decoder_env.decoder_list);
    while (tmp) {
        uint32_t distance;
        uint32_t wr_ptr = tmp->wr_ptr;
        if (wr_ptr == audio_decoder_env.wr_ptr) {
            min_distance = 0;
            break;
        }
        else if (wr_ptr > audio_decoder_env.wr_ptr) {
            distance = wr_ptr - audio_decoder_env.wr_ptr;
        }
        else {
            distance = audio_decoder_env.pcm_total_samples + wr_ptr - audio_decoder_env.wr_ptr;
        }

        min_distance = min_distance < distance ? min_distance : distance;

        tmp = (void *)tmp->hdr.next;
    }
    GLOBAL_INT_RESTORE();

    if ((min_distance != 0) && (min_distance != 0xffffffff)){
        audio_decoder_env.wr_ptr += min_distance;
        if (audio_decoder_env.wr_ptr >= audio_decoder_env.pcm_total_samples) {
            audio_decoder_env.wr_ptr -= audio_decoder_env.pcm_total_samples;
        }

        ret = true;
    }
    else {
        ret = false;
    }
    
//    printf("update_wr_ptr: ");
//    print_int16(audio_decoder_env.wr_ptr);
//    fputc(' ', NULL);
//    tmp = (void *)co_list_pick(&audio_decoder_env.decoder_list);
//    while (tmp) {
//        print_int16(tmp->wr_ptr);
//        fputc(' ', NULL);
//        tmp = (void *)tmp->hdr.next;
//    }
//    print_int16(audio_decoder_env.rd_ptr);
//    printf("\r\n");
    
    return ret;
}

/* update read pointer after PCM is fetched by an output or any output is removed */
static bool update_rd_ptr(void)
{
    audio_decoder_output_t *tmp;
    uint32_t min_distance = 0xffffffff;
    bool ret;
    uint32_t curr_rd_ptr;

    GLOBAL_INT_DISABLE();
    curr_rd_ptr = audio_decoder_env.rd_ptr;
    /* update rd_ptr according to minimum distance */
    tmp = (void *)co_list_pick(&audio_decoder_env.output_list);
    while (tmp) {
        uint32_t distance, rd_ptr;
        rd_ptr = tmp->rd_ptr;
        if (rd_ptr == curr_rd_ptr) {
            min_distance = 0;
            break;
        }
        else if (rd_ptr > curr_rd_ptr) {
            distance = rd_ptr - curr_rd_ptr;
        }
        else {
            distance = audio_decoder_env.pcm_total_samples + rd_ptr - curr_rd_ptr;
        }

        min_distance = min_distance < distance ? min_distance : distance;

        tmp = (void *)tmp->hdr.next;
    }

    if (min_distance == 0) {
        ret = false;
    }
    else {
        audio_decoder_env.rd_ptr = curr_rd_ptr + min_distance;
        if (audio_decoder_env.rd_ptr >= audio_decoder_env.pcm_total_samples) {
            audio_decoder_env.rd_ptr -= audio_decoder_env.pcm_total_samples;
        }
        ret = true;
    }
    GLOBAL_INT_RESTORE();
    
//    audio_decoder_output_t *_tmp;
//    printf("rd: ");
//    print_int16(audio_decoder_env.rd_ptr);
//    _tmp = (void *)co_list_pick(&audio_decoder_env.output_list);
//    while (_tmp) {
//        print_int16(_tmp->rd_ptr);
//        _tmp = (void *)_tmp->hdr.next;
//    }
//    printf("\r\n");
    
    return ret;
}

static audio_ret_t pcm_buffer_status(uint32_t wr_ptr)
{
    uint32_t available_space;
    uint32_t wr_limit = AUDIO_DECODER_MIXED_STORE_UPPER;

    if (wr_ptr >= wr_limit) {
        available_space = wr_limit + audio_decoder_env.pcm_total_samples - wr_ptr;
    }
    else {
        available_space = wr_limit - wr_ptr;
    }

    if (available_space >= audio_decoder_env.request_data_thd) {
        return AUDIO_RET_NEED_MORE;
    }
    else if (available_space < audio_decoder_env.mixed_buffer_almost_full_thd) {
        return AUDIO_RET_OUTPUT_ALMOTE_FULL;
    }
    else {
        return AUDIO_RET_OK;
    }
}

/* Request decoder directly. this function will be called in audio_decoder_get_pcm when decoder is not started */
static void request_raw_data(void)
{
    audio_decoder_t *tmp;
    audio_ret_t ret;

    GLOBAL_INT_DISABLE();
    tmp = (void *)co_list_pick(&audio_decoder_env.decoder_list);
    while (tmp) {
        if (tmp->state == AUDIO_DECODER_STATE_DECODING) {
            if (tmp->evt_cb) {
                tmp->evt_cb(tmp, AUDIO_DECODER_EVENT_REQ_RAW_DATA);
            }
        }

        tmp = (void *)tmp->hdr.next;
    }
    GLOBAL_INT_RESTORE();
}

/* After rd_ptr is updated, new space is available for decoder */
static void check_request_raw_data(void)
{
    audio_decoder_t *tmp;
    audio_ret_t ret;

    GLOBAL_INT_DISABLE();
    tmp = (void *)co_list_pick(&audio_decoder_env.decoder_list);
    while (tmp) {
        if (tmp->state == AUDIO_DECODER_STATE_DECODING) {
            ret = pcm_buffer_status(tmp->wr_ptr);
            if (ret == AUDIO_RET_NEED_MORE) {
                if (tmp->evt_cb) {
                    tmp->evt_cb(tmp, AUDIO_DECODER_EVENT_REQ_RAW_DATA);
                }
            }
        }else if(tmp->state == AUDIO_DECODER_STATE_INPUT_OVER) {
            mix_decoded_data(tmp);
        }
        tmp = (void *)tmp->hdr.next;
    }
    GLOBAL_INT_RESTORE();
}

static void *add_to_mixed_buffer(int16_t *mixed_pcm_ptr, int16_t *src, uint32_t samples)
{
    if (audio_decoder_env.out_ch_num == 1) {
        for (uint32_t i=0; i<samples; i++) {
            *mixed_pcm_ptr += *src++;
            mixed_pcm_ptr++;
        }
    }
    else {
        for (uint32_t i=0; i<samples; i++) {
            *mixed_pcm_ptr += *src++;
            mixed_pcm_ptr++;
            *mixed_pcm_ptr += *src++;
            mixed_pcm_ptr++;
        }
    }
    
    return src;
}

static void *copy_to_mixed_buffer(int16_t *mixed_pcm_ptr, int16_t *src, uint32_t samples)
{
    if (audio_decoder_env.out_ch_num == 1) {
        for (uint32_t i=0; i<samples; i++) {
            *mixed_pcm_ptr++ = *src++;
        }
    }
    else {
        for (uint32_t i=0; i<samples; i++) {
            *mixed_pcm_ptr++ = *src++;
            *mixed_pcm_ptr++ = *src++;
        }
    }
    
    return src;
}

static uint32_t save_data_to_mixed_buffer(uint32_t add_start_index, uint32_t copy_start_index, 
                                                    uint32_t end_index, int16_t *pcm, uint32_t samples)
{
    uint32_t copy_samples = 0, add_samples = 0;
    uint32_t dealed_samples;
    uint32_t dealing_index;
    int16_t *mixed_pcm_ptr;
    
//    printf("save_data_to_mixed_buffer: ");
//    print_int16(add_start_index);
//    fputc(' ', NULL);
//    print_int16(copy_start_index);
//    fputc(' ', NULL);
//    print_int16(end_index);
//    fputc(' ', NULL);
//    print_int16(samples);
//    fputc(' ', NULL);
//    printf("\r\n");

    if (copy_start_index >= add_start_index) {
        add_samples = copy_start_index - add_start_index;
    }
    else {
        add_samples = copy_start_index + audio_decoder_env.pcm_total_samples - add_start_index;
    }

    if (end_index == 0) {
        end_index = audio_decoder_env.pcm_total_samples - 1;
    }
    else {
        end_index--;
    }
    if (samples > add_samples) {
        if (end_index >= copy_start_index) {
            copy_samples = end_index - copy_start_index;
        }
        else {
            copy_samples = end_index + audio_decoder_env.pcm_total_samples - copy_start_index;
        }

        if (copy_samples > (samples - add_samples)) {
            copy_samples = samples - add_samples;
        }
    }
    else {
        add_samples = samples;
    }

    dealed_samples = copy_samples + add_samples;

    dealing_index = add_start_index;
    mixed_pcm_ptr = &audio_decoder_env.pcm[add_start_index * audio_decoder_env.out_ch_num];
    if (add_samples) {
        uint32_t last_samples = audio_decoder_env.pcm_total_samples - dealing_index;
        if (last_samples <= add_samples) {
            pcm = add_to_mixed_buffer(mixed_pcm_ptr, pcm, last_samples);
            add_samples -= last_samples;
            dealing_index = 0;
            mixed_pcm_ptr = &audio_decoder_env.pcm[0];
        }

        if (add_samples) {
            pcm = add_to_mixed_buffer(mixed_pcm_ptr, pcm, add_samples);
        }
    }

    dealing_index = copy_start_index;
    mixed_pcm_ptr = &audio_decoder_env.pcm[copy_start_index * audio_decoder_env.out_ch_num];
    if (copy_samples) {
        uint32_t last_samples = audio_decoder_env.pcm_total_samples - dealing_index;
        if (last_samples <= copy_samples) {
            pcm = copy_to_mixed_buffer(mixed_pcm_ptr, pcm, last_samples);
            copy_samples -= last_samples;
            dealing_index = 0;
            mixed_pcm_ptr = &audio_decoder_env.pcm[0];
        }

        if (copy_samples) {
            pcm = copy_to_mixed_buffer(mixed_pcm_ptr, pcm, copy_samples);
        }
    }

    return dealed_samples;
}

/* update mixed PCM buffer with PCM data from pcm list of coresponding decoder. */
static void mix_decoded_data(audio_decoder_t *decoder)
{
    uint32_t max_distance;
    uint32_t min_distance = 0;
    uint32_t fastest_wr_ptr = 0;
    audio_decoder_t *tmp;
    audio_decoder_pcm_data_t *pcm_data;

    pcm_data = (void *)co_list_pick(&decoder->pcm_list);
    if (pcm_data == NULL) {
        return;
    }
    
//    vTaskSuspendAll();

    /* search the fastest wr_ptr in decoder list */
    max_distance = 0;
    fastest_wr_ptr = audio_decoder_env.wr_ptr;
    GLOBAL_INT_DISABLE();
    tmp = (void *)co_list_pick(&audio_decoder_env.decoder_list);
    while (tmp) {
        uint32_t distance;
        if (tmp->wr_ptr >= audio_decoder_env.wr_ptr) {
            distance = tmp->wr_ptr - audio_decoder_env.wr_ptr;
        }
        else {
            distance = audio_decoder_env.pcm_total_samples - audio_decoder_env.wr_ptr + tmp->wr_ptr;
        }

        if (max_distance < distance) {
            max_distance = distance;
            fastest_wr_ptr = tmp->wr_ptr;
        }

        tmp = (void *)tmp->hdr.next;
    }
    GLOBAL_INT_RESTORE();

    /* save incomming decoded data into mixed buffer */
    pcm_data = (void *)co_list_pick(&decoder->pcm_list);
    while (pcm_data) {
        uint32_t dealed_samples;
        uint32_t distance;
        dealed_samples = save_data_to_mixed_buffer(decoder->wr_ptr, 
                                                    fastest_wr_ptr, 
                                                    AUDIO_DECODER_MIXED_STORE_UPPER, 
                                                    &pcm_data->pcm[pcm_data->offset*audio_decoder_env.out_ch_num], 
                                                    pcm_data->samples - pcm_data->offset);
        pcm_data->offset += dealed_samples;
        decoder->wr_ptr += dealed_samples;
        if (decoder->wr_ptr >= audio_decoder_env.pcm_total_samples) {
            decoder->wr_ptr -= audio_decoder_env.pcm_total_samples;
        }
        if (pcm_data->offset > pcm_data->samples) {
            __disable_irq();
            while(1);
        }
        if (pcm_data->offset == pcm_data->samples) {
            co_list_pop_front(&decoder->pcm_list);
            vPortFree(pcm_data);
            pcm_data = (void *)co_list_pick(&decoder->pcm_list);
            
            /* update max_distance for following processing */
            if (decoder->wr_ptr >= audio_decoder_env.wr_ptr) {
                distance = decoder->wr_ptr - audio_decoder_env.wr_ptr;
            }
            else {
                distance = audio_decoder_env.pcm_total_samples - audio_decoder_env.wr_ptr + decoder->wr_ptr;
            }
            if (distance > max_distance) {
                fastest_wr_ptr = decoder->wr_ptr;
            }
        }
        else {
            break;
        }
    }
    
    if (decoder->state == AUDIO_DECODER_STATE_INPUT_OVER) {
        if (co_list_is_empty(&decoder->pcm_list)) {
            GLOBAL_INT_DISABLE();
            audio_decoder_stop(decoder);
            co_list_extract(&audio_decoder_env.decoder_wait_start_list, &decoder->hdr);
            decoder->state = AUDIO_DECODER_STATE_PCM_ALL_MIXED;
            co_list_push_back(&audio_decoder_env.decoder_wait_pcm_consumed_list, &decoder->hdr);
            GLOBAL_INT_RESTORE();
        }
    }

    update_wr_ptr();
    
//    ( void ) xTaskResumeAll();
}

/* 
 * save PCM data into pcm list of corresponding decoder, the sample rate 
 * and channels of saved PCM data are the same with settings in 
 * audio_decoder_env
 */
static void save_decoded_data(audio_decoder_t *decoder, uint8_t *decoder_out_ptr, uint32_t decoder_out_length, uint8_t channels)
{    
    audio_decoder_pcm_data_t *pcm_data;
    int16_t *src, *dst;
    uint32_t sample_count;
    uint16_t ratio;
    int16_t value;

    ratio = ratio_table[audio_decoder_env.decoder_count - 1];

    if (channels == audio_decoder_env.out_ch_num) {
#if FIX_44100_TEMP
        if (add_sample) {
            decoder_out_length += 4;
        }
#endif
        pcm_data = pvPortMalloc(sizeof(audio_decoder_pcm_data_t) + decoder_out_length);
        sample_count = decoder_out_length / (channels * sizeof(int16_t));
        pcm_data->samples = sample_count;
        pcm_data->offset = 0;
        if (audio_decoder_env.decoder_count == 1) {
            memcpy((void *)&pcm_data->pcm[0], decoder_out_ptr, decoder_out_length);
#if FIX_44100_TEMP
            if (add_sample) {
                add_sample = false;
                memcpy((void *)&pcm_data->pcm[(decoder_out_length-4)>>1], (void *)&pcm_data->pcm[(decoder_out_length-8)>>1], 4);
            }
#endif
        }
        else {
            src = (void *)decoder_out_ptr;
            dst = (void *)&pcm_data->pcm[0];
            for (uint32_t i=0; i<sample_count * channels; i++) {
                value = (((*src++) * ratio) >> 16);
                *dst++ = value;
            }
        }
    }
    else {
        if (channels == 1) {
            /* source: mono; destination: stereo */
            pcm_data = pvPortMalloc(sizeof(audio_decoder_pcm_data_t) + decoder_out_length * 2);
            sample_count = decoder_out_length / sizeof(int16_t);
            pcm_data->samples = sample_count;
            pcm_data->offset = 0;

            src = (void *)decoder_out_ptr;
            dst = (void *)&pcm_data->pcm[0];
            if (audio_decoder_env.decoder_count == 1) {
                for (uint32_t i=0; i<sample_count; i++) {
                    *dst++ = *src;
                    *dst++ = *src++;
                }
            }
            else {
                for (uint32_t i=0; i<sample_count; i++) {
                    value = (((*src++) * ratio) >> 16);
                    *dst++ = value;
                    *dst++ = value;
                }
            }
        }
        else {
            /* source: stereo; destination: mono */
            pcm_data = pvPortMalloc(sizeof(audio_decoder_pcm_data_t) + decoder_out_length / 2);
            sample_count = decoder_out_length / sizeof(int16_t) / 2;
            pcm_data->samples = sample_count;
            pcm_data->offset = 0;

            src = (void *)decoder_out_ptr;
            dst = (void *)&pcm_data->pcm[0];
            if (audio_decoder_env.decoder_count == 1) {
                for (uint32_t i=0; i<sample_count; i++) {
                    *dst++ = *src;
                    src += 2;
                }
            }
            else {
                for (uint32_t i=0; i<sample_count; i++) {
                    value = (((*src) * ratio) >> 16);
                    *dst++ = value;
                    src += 2;
                }
            }
        }
    }

    GLOBAL_INT_DISABLE();
    co_list_push_back(&decoder->pcm_list, &pcm_data->hdr);
    GLOBAL_INT_RESTORE();
}

void audio_decoder_start(audio_decoder_t *decoder)
{
    uint32_t available_samples;
    uint32_t mini_vacuumn_sample_num;
    uint32_t max_distance;
    uint32_t fastest_wr_ptr = 0;
    uint32_t tmp_wr_ptr = 0;
    audio_decoder_t *tmp = NULL;
    bool extracted = false;
    
    GLOBAL_INT_DISABLE();
        
    if(!co_list_find(&audio_decoder_env.decoder_list, &decoder->hdr))
    {
        /* search the fastest wr_ptr in decoder list */
        max_distance = 0;
        fastest_wr_ptr = audio_decoder_env.wr_ptr;
        tmp = (void *)co_list_pick(&audio_decoder_env.decoder_list);
        while (tmp) {
            uint32_t distance;
            if (tmp->wr_ptr >= audio_decoder_env.wr_ptr) {
                distance = tmp->wr_ptr - audio_decoder_env.wr_ptr;
            }
            else {
                distance = audio_decoder_env.pcm_total_samples - audio_decoder_env.wr_ptr + tmp->wr_ptr;
            }
            // max_distance = max_distance > distance ? max_distance : distance;
            if (max_distance < distance) {
                max_distance = distance;
                fastest_wr_ptr = tmp->wr_ptr;
            }

        tmp = (void *)tmp->hdr.next;
    }

        /*when start a decoder, it's wr_ptr should equal to the fastest wr_ptr in the decoder_env.
        the value would be audio_decoder_env.wr_ptr when there is no decoder instance.*/
        decoder->wr_ptr = fastest_wr_ptr;
        /*In case that one decoder was stoped due to data insufficient, when restart it, valid PCM samples should be more than mini_vacuumn_sample_num,
        audio_decoder_get_pcm() from interrupt is likely to stop the decoder before it decodes valid PCM samples successfully otherwise.
        */
        if (fastest_wr_ptr >= audio_decoder_env.rd_ptr) {
            available_samples = fastest_wr_ptr - audio_decoder_env.rd_ptr;
        }
        else {
            available_samples = audio_decoder_env.pcm_total_samples - (audio_decoder_env.rd_ptr - fastest_wr_ptr);
        }

        mini_vacuumn_sample_num =  audio_decoder_env.out_sample_rate * AUDIO_DECODER_PCM_RSV_AT_BEGINNING / 1000;

        if(available_samples < mini_vacuumn_sample_num)
        {
            tmp_wr_ptr = (audio_decoder_env.rd_ptr + mini_vacuumn_sample_num);

            if (tmp_wr_ptr >= audio_decoder_env.pcm_total_samples) {
                tmp_wr_ptr -= audio_decoder_env.pcm_total_samples;
            }

            if(tmp_wr_ptr > fastest_wr_ptr)
            {
                memset((void *)&audio_decoder_env.pcm[fastest_wr_ptr * audio_decoder_env.out_ch_num], 
                        0, sizeof(int16_t) * audio_decoder_env.out_ch_num * (tmp_wr_ptr - fastest_wr_ptr));

            }else{
                memset((void *)&audio_decoder_env.pcm[fastest_wr_ptr * audio_decoder_env.out_ch_num], 
                        0, sizeof(int16_t) * audio_decoder_env.out_ch_num * (audio_decoder_env.pcm_total_samples - fastest_wr_ptr));
                memset((void *)&audio_decoder_env.pcm[0], 
                        0, sizeof(int16_t) * audio_decoder_env.out_ch_num * tmp_wr_ptr);
            }
            decoder->wr_ptr = tmp_wr_ptr;
        }

        extracted = co_list_extract(&audio_decoder_env.decoder_wait_start_list, &decoder->hdr);
        if (extracted) {
            co_list_push_back(&audio_decoder_env.decoder_list, &decoder->hdr);
            decoder->state = AUDIO_DECODER_STATE_DECODING;
            audio_decoder_env.decoder_count++;
        }else{
            assert(extracted == true);
        }
        update_wr_ptr();
    }
    GLOBAL_INT_RESTORE();
}

void audio_decoder_stop(audio_decoder_t *decoder)
{
    bool extracted = false;
    
    GLOBAL_INT_DISABLE();
    extracted = co_list_extract(&audio_decoder_env.decoder_list, &decoder->hdr);
    if (extracted) {
        audio_decoder_env.decoder_count--;
        update_wr_ptr();
        co_list_push_back(&audio_decoder_env.decoder_wait_start_list, &decoder->hdr);
        decoder->state = AUDIO_DECODER_STATE_IDLE;
    }
    GLOBAL_INT_RESTORE();
}

audio_decoder_t *audio_decoder_add(audio_type_t type, audio_decoder_param_t *param, void (*evt_cb)(audio_decoder_t *, uint8_t event))
{
    audio_decoder_t *decoder;

    if (audio_decoder_env.inited == false) {
        return NULL;
    }

    decoder = pvPortMalloc(sizeof(audio_decoder_t));
    if (decoder) {
        switch (type) {
            case AUDIO_TYPE_SBC:
                decoder->decoder = codec_decoder_init(CODEC_DECODER_TYPE_SBC, NULL);
                break;
            case AUDIO_TYPE_AAC:
                {
                    decoder->decoder = codec_decoder_init(CODEC_DECODER_TYPE_AAC, (void *)&param->aac);
                }
                break;
            case AUDIO_TYPE_MP3:
                decoder->decoder = codec_decoder_init(CODEC_DECODER_TYPE_MP3, NULL);
                break;
            case AUDIO_TYPE_MSBC:
                decoder->decoder = codec_decoder_init(CODEC_DECODER_TYPE_MSBC, NULL);
                break;
            case AUDIO_TYPE_CVSD:
                {
                    decoder->decoder = codec_decoder_init(CODEC_DECODER_TYPE_CVSD, (void *)&param->cvsd);
                }
                break;
            case AUDIO_TYPE_LC3:
                break;
            case AUDIO_TYPE_PCM:
                {
                    decoder->decoder = codec_decoder_init(CODEC_DECODER_TYPE_PCM, (void *)&param->pcm);
                }
                break;
            case AUDIO_TYPE_SBC_V2:
                decoder->decoder = codec_decoder_init(CODEC_DECODER_TYPE_SBC_V2, NULL);
                break;
            case AUDIO_TYPE_OPUS_V2:
                {
                    decoder->decoder = codec_decoder_init(CODEC_DECODER_TYPE_OPUS_V2, (void *)&param->opus_v2);
                }
                break;
            default:
                vPortFree(decoder);
                return NULL;
        }

        decoder->evt_cb = evt_cb;
        decoder->current_sample_rate = 0;
        decoder->resample = NULL;
        decoder->wr_ptr = 0;

        co_list_init(&decoder->pcm_list);
        decoder->state = AUDIO_DECODER_STATE_IDLE;
        GLOBAL_INT_DISABLE();
        co_list_push_back(&audio_decoder_env.decoder_wait_start_list, &decoder->hdr);
        GLOBAL_INT_RESTORE();
    }

    return decoder;
}

void audio_decoder_remove(audio_decoder_t *decoder)
{
    if (decoder == NULL) {
        return;
    }
    audio_decoder_pcm_data_t *pcm_data;
    
    GLOBAL_INT_DISABLE();
    audio_decoder_stop(decoder);
    co_list_extract(&audio_decoder_env.decoder_wait_start_list, &decoder->hdr);
    co_list_extract(&audio_decoder_env.decoder_wait_to_be_destroyed, &decoder->hdr);
    co_list_extract(&audio_decoder_env.decoder_wait_pcm_consumed_list, &decoder->hdr);
    GLOBAL_INT_RESTORE();

    do {
        pcm_data = (void *)co_list_pop_front(&decoder->pcm_list);
        if (pcm_data) {
            vPortFree(pcm_data);
        }
    } while(pcm_data);

    if (decoder->resample) {
        resample_destroy(decoder->resample);
    }
    codec_decoder_destroy(decoder->decoder);

    vPortFree(decoder);
}

audio_decoder_output_t *audio_decoder_output_add(uint8_t immediate, uint8_t channels)
{
    if ((audio_decoder_env.inited == false)
        || (audio_decoder_env.out_ch_num != channels)) {
        return NULL;
    }

    audio_decoder_output_t *output = pvPortMalloc(sizeof(audio_decoder_output_t));
    if (output) {
        output->immediate = immediate;
        output->missed_samples = 0;
        GLOBAL_INT_DISABLE();
        output->rd_ptr = audio_decoder_env.rd_ptr;
        co_list_push_back(&audio_decoder_env.output_list, &output->hdr);
        GLOBAL_INT_RESTORE();
    }

    return output;
}

void audio_decoder_output_remove(audio_decoder_output_t *output)
{
    if (output == NULL) {
        return;
    }

    GLOBAL_INT_DISABLE();
    if (co_list_extract(&audio_decoder_env.output_list, &output->hdr)) {
        if (update_rd_ptr()) {
            check_request_raw_data();
        }
    }
    vPortFree(output);
    GLOBAL_INT_RESTORE();
}

int audio_decoder_decode(audio_decoder_t *decoder, const uint8_t *buffer, uint32_t *length)
{
    uint32_t decoder_in_length;
    uint8_t *decoder_out_ptr;
    uint32_t decoder_out_length;
    uint32_t input_length, consumed_length;
    
    if (decoder->state != AUDIO_DECODER_STATE_DECODING) {
        *length = 0;
        return AUDIO_RET_OUTPUT_ALMOTE_FULL;
    }
    
    if (co_list_pick(&decoder->pcm_list)) {
        audio_ret_t ret;

        mix_decoded_data(decoder);
        ret = pcm_buffer_status(decoder->wr_ptr);
        if (ret == AUDIO_RET_OUTPUT_ALMOTE_FULL) {
            *length = 0;
            return ret;
        }
    }

    input_length = *length;
    consumed_length = 0;
    while (input_length) {

        decoder_in_length = input_length;
        decoder_out_ptr = NULL;
        if (input_length == AUDIO_SPECIAL_LENGTH_FOR_INPUT_OVER) {
            uint8_t exec_done;
            codec_decoder_input_over(decoder->decoder, &decoder_out_ptr, &decoder_out_length, &exec_done);
            if (exec_done) {
                decoder_in_length = input_length;
                decoder->state = AUDIO_DECODER_STATE_INPUT_OVER;
            }
            else {
                decoder_in_length = 0;
            }
        }
        else if (input_length == AUDIO_SPECIAL_LENGTH_FOR_PLC) {
            codec_decoder_plc(decoder->decoder, &decoder_out_ptr, &decoder_out_length);
            decoder_in_length = input_length;
        }
        else {
            codec_decoder_decode(decoder->decoder, buffer, &decoder_in_length, &decoder_out_ptr, &decoder_out_length);
        }
        
        if (decoder_out_length) 
        {
            uint32_t sample_rate;
            uint8_t channels;
//            uint8_t *ptr = decoder_out_ptr;
//            for(uint32_t i=0;i<decoder_out_length;)
//            {
//                printf("%02x", ptr[i]);
//                i++;
//                if((i%128) == 0)
//                {
//                    printf("\n");
//                }
//            }

            if (input_length != AUDIO_SPECIAL_LENGTH_FOR_PLC) {
                codec_decoder_get_param(decoder->decoder, &sample_rate, &channels);
            }
            else {
                sample_rate = decoder->current_sample_rate;
                channels = audio_decoder_env.out_ch_num;
            }
            
#if FIX_44100_TEMP
            if (sample_rate == 44100) {
                record_samples += decoder_out_length;
                if (record_samples >= 2450*4) {
                    add_sample = true;
                    record_samples -= 2450*4;
                }
            }
#endif

            if (decoder->current_sample_rate != sample_rate) {
                decoder->current_sample_rate = sample_rate;

                if (sample_rate == audio_decoder_env.out_sample_rate) {
                    if (decoder->resample) {
                        resample_destroy(decoder->resample);
                    }
                    decoder->resample = 0;
                }
                else {
                    enum resample_type type;

                    if (decoder->resample) {
                        resample_destroy(decoder->resample);
                    }

                    type = resample_get_type(sample_rate, audio_decoder_env.out_sample_rate);
                    if (type != RESAMPLE_TYPE_INVALID) {
                        decoder->resample = resample_init(type, channels);
                    }
                    else {
                        /// TODO
                        decoder->resample = 0;
                    }
                }
            }

            if (decoder->resample) {
                while (decoder_out_length) {
                    uint32_t dealed_length = decoder_out_length;
                    uint32_t out_length;
                    uint8_t *out_buf = NULL;
                    resample_exec(decoder->resample, 
                                    (const uint8_t *)&decoder_out_ptr[0], 
                                    &dealed_length, 
                                    &out_buf, 
                                    &out_length);
                    if (out_length) {
                        save_decoded_data(decoder, out_buf, out_length, channels);
                    }
                    decoder_out_length -= dealed_length;
                    decoder_out_ptr += dealed_length;
                }
            }
            else {
                save_decoded_data(decoder, decoder_out_ptr, decoder_out_length, channels);
            }

            mix_decoded_data(decoder);
        }
        else {
            if (input_length == AUDIO_SPECIAL_LENGTH_FOR_INPUT_OVER) {
                if (co_list_is_empty(&decoder->pcm_list)) {
                    GLOBAL_INT_DISABLE();
                    audio_decoder_stop(decoder);
                    co_list_extract(&audio_decoder_env.decoder_wait_start_list, &decoder->hdr);
                    decoder->state = AUDIO_DECODER_STATE_PCM_ALL_MIXED;
                    co_list_push_back(&audio_decoder_env.decoder_wait_pcm_consumed_list, &decoder->hdr);
                    GLOBAL_INT_RESTORE();
                }
            }
        }

        input_length -= decoder_in_length;
        buffer += decoder_in_length;
        consumed_length += decoder_in_length;

    }
    *length = consumed_length;

    return pcm_buffer_status(decoder->wr_ptr);
}

static int16_t *copy_pcm(int16_t *dst, int16_t *src, uint32_t samples, uint8_t channels)
{
    if (audio_decoder_env.out_ch_num == AUDIO_CHANNELS_MONO) {
        if (channels == AUDIO_CHANNELS_MONO) {
            for (uint32_t i=0; i<samples; i++) {
                *dst++ = *src++;
            }
        }
        else {
            for (uint32_t i=0; i<samples; i++) {
                *dst++ = *src;
                *dst++ = *src++;
            }
        }
    }
    else {
        if (channels == AUDIO_CHANNELS_MONO) {
            for (uint32_t i=0; i<samples; i++) {
                *dst++ = *src;
                src += 2 ;
            }
        }
        else {
            uint32_t *_dst, *_src;
            _dst = (void *)dst;
            _src = (void *)src;
            for (uint32_t i=0; i<samples; i++) {
                *_dst++ = *_src++;
            }
            dst = (void *)_dst;
        }
    }
    
    return dst;
}

uint32_t audio_decoder_get_pcm(audio_decoder_output_t *output, int16_t *pcm, uint32_t samples, uint8_t channels)
{
    uint32_t available_samples, tail_samples, fill_zero_samples, valid_samples;
    int16_t *mixed_pcm_ptr;
    uint32_t current_wr_ptr;

    if ((audio_decoder_env.inited == false)
            || ((audio_decoder_env.decoder_count == 0) && co_list_is_empty(&audio_decoder_env.decoder_wait_pcm_consumed_list))
            || (output == NULL)) {
        for (uint32_t i=0; i<samples * channels; i++) {
            *pcm++ = 0;
        }
        return samples;
    }

    current_wr_ptr = audio_decoder_env.wr_ptr;
    if (current_wr_ptr >= output->rd_ptr) {
        available_samples = current_wr_ptr - output->rd_ptr;
    }
    else {
        available_samples = current_wr_ptr + audio_decoder_env.pcm_total_samples - output->rd_ptr;
    }
    
    if (output->missed_samples) {
        uint32_t discard_samples;
        if (output->missed_samples < available_samples) {
            discard_samples = output->missed_samples;
        }
        else {
            discard_samples = available_samples;
        }
        output->rd_ptr += discard_samples;
        if (output->rd_ptr >= audio_decoder_env.pcm_total_samples) {
            output->rd_ptr -= audio_decoder_env.pcm_total_samples;
        }
        available_samples -= discard_samples;
        output->missed_samples -= discard_samples;
    }

    if (available_samples > samples) {
        available_samples = samples;
        fill_zero_samples = 0;
        valid_samples = samples;
    }
    else {
        if (output->immediate) {
            fill_zero_samples = samples - available_samples;
            valid_samples = samples;
        }
        else {
            fill_zero_samples = 0;
            valid_samples = available_samples;
        }
    }

    mixed_pcm_ptr = &audio_decoder_env.pcm[output->rd_ptr * audio_decoder_env.out_ch_num];
    tail_samples = audio_decoder_env.pcm_total_samples - output->rd_ptr;
    if (available_samples >= tail_samples) {
        pcm = copy_pcm(pcm, mixed_pcm_ptr, tail_samples, channels);
        available_samples -= tail_samples;
        output->rd_ptr = 0;
        mixed_pcm_ptr = &audio_decoder_env.pcm[0];
    }

    if (available_samples) {
        pcm = copy_pcm(pcm, mixed_pcm_ptr, available_samples, channels);
        output->rd_ptr += available_samples;
    }

    if (fill_zero_samples) {
        output->missed_samples += fill_zero_samples;
        if (channels == AUDIO_CHANNELS_MONO) {
            for (uint32_t i=0; i<fill_zero_samples; i++) {
                *pcm++ = 0;
            }
        }
        else {
            uint32_t *dst;
            dst = (void *)pcm;
            for (uint32_t i=0; i<fill_zero_samples; i++) {
                *dst++ = 0;
            }
        }
    }

    /*
     * 1. read pointer is updated, more space can be used to store decoded data
     * 2. immediate is true, request more data when fill_zero_samples is not 0.
     * 3. immediate is false, valid_samples is less than request size
     */
    if (update_rd_ptr() || fill_zero_samples || (valid_samples < samples)) {
        check_request_raw_data();
    }
    
    GLOBAL_INT_DISABLE();
    {
        audio_decoder_t *decoder;
        decoder = (void *)co_list_pick(&audio_decoder_env.decoder_wait_pcm_consumed_list);
        while (decoder) {
            uint32_t distance;
            if (audio_decoder_env.rd_ptr >= decoder->wr_ptr) {
                distance = decoder->wr_ptr + audio_decoder_env.pcm_total_samples - audio_decoder_env.rd_ptr;
            }
            else {
                distance = decoder->wr_ptr - audio_decoder_env.rd_ptr;
            }
            if (distance <= samples) {
                void *next = decoder->hdr.next;
                if (decoder->evt_cb) {
                    decoder->evt_cb(decoder, AUDIO_DECODER_EVENT_PCM_CONSUMED);
                }
                co_list_extract(&audio_decoder_env.decoder_wait_pcm_consumed_list, &decoder->hdr);
                decoder->state = AUDIO_DECODER_STATE_IDLE;
                co_list_push_back(&audio_decoder_env.decoder_wait_to_be_destroyed, &decoder->hdr);
                decoder = next;
            }
            else {
                decoder = (void *)decoder->hdr.next;
            }
        }
    }
    GLOBAL_INT_RESTORE();

    return valid_samples;
}

bool audio_decoder_is_started(audio_decoder_t *decoder)
{
    if (decoder) {
        return decoder->state == AUDIO_DECODER_STATE_DECODING;
    }
    else {
        return false;
    }
}

uint32_t audio_decoder_get_missed_sample_cnt(audio_decoder_output_t *output)
{
    return output->missed_samples;
}

void audio_decoder_clear_missed_sample_cnt(audio_decoder_output_t *output)
{
    output->missed_samples = 0;
}

int audio_decoder_init(uint8_t out_ch_num, uint32_t out_sample_rate)
{
    if (audio_decoder_env.inited) {
        return AUDIO_RET_ERR_CREATED;
    }

    audio_decoder_env.decoder_count = 0;
    audio_decoder_env.out_ch_num = out_ch_num;
    audio_decoder_env.out_sample_rate = out_sample_rate;
    co_list_init(&audio_decoder_env.output_list);
    co_list_init(&audio_decoder_env.decoder_list);
    co_list_init(&audio_decoder_env.decoder_wait_start_list);
    co_list_init(&audio_decoder_env.decoder_wait_pcm_consumed_list);
    co_list_init(&audio_decoder_env.decoder_wait_to_be_destroyed);

    audio_decoder_env.pcm_total_samples = out_sample_rate * AUDIO_DECODER_PCM_MIXED_BUFFER_DUR / 1000;
#if AUDIO_DECODER_USER_DRAM == 0
    audio_decoder_env.pcm = (void *)pvPortMalloc(audio_decoder_env.pcm_total_samples * out_ch_num * sizeof(int16_t));
#else
    audio_decoder_env.pcm = (void *)&decoder_pcm[0];
#endif
    audio_decoder_env.wr_ptr = 0;
    #if AUDIO_DECODER_PCM_RSV_AT_BEGINNING != 0
    audio_decoder_env.rd_ptr = out_sample_rate * (AUDIO_DECODER_PCM_MIXED_BUFFER_DUR - AUDIO_DECODER_PCM_RSV_AT_BEGINNING) / 1000;
    memset((void *)&audio_decoder_env.pcm[audio_decoder_env.rd_ptr * out_ch_num], 0, sizeof(int16_t) * out_ch_num * (audio_decoder_env.pcm_total_samples - audio_decoder_env.rd_ptr));
    #else
    audio_decoder_env.rd_ptr = 0;
    #endif

    audio_decoder_env.request_data_thd = out_sample_rate * AUDIO_DECODER_REQUEST_DATA_THD / 1000;
    audio_decoder_env.mixed_buffer_almost_full_thd = out_sample_rate * AUDIO_DECODER_PCM_MIXED_BUFFER_FULL_THD / 1000;

    audio_decoder_env.inited = true;

#if FIX_44100_TEMP
    add_sample = false;
    record_samples = 0;
#endif

    return AUDIO_RET_OK;
}

void audio_decoder_destroy(void)
{
    audio_decoder_t *decoder;
    audio_decoder_output_t *output;

    if ( audio_decoder_env.inited== false) {
        return;
    }

    audio_decoder_env.inited = false;

    do {
        decoder = (void *)co_list_pick(&audio_decoder_env.decoder_wait_start_list);
        if (decoder) {
            audio_decoder_remove(decoder);
        }
    } while(decoder);
    do {
        decoder = (void *)co_list_pick(&audio_decoder_env.decoder_list);
        if (decoder) {
            audio_decoder_remove(decoder);
        }
    } while(decoder);
    do {
        decoder = (void *)co_list_pick(&audio_decoder_env.decoder_wait_pcm_consumed_list);
        if (decoder) {
            audio_decoder_remove(decoder);
        }
    } while(decoder);
    do {
        decoder = (void *)co_list_pick(&audio_decoder_env.decoder_wait_to_be_destroyed);
        if (decoder) {
            audio_decoder_remove(decoder);
        }
    } while(decoder);

    do {
        output = (void *)co_list_pick(&audio_decoder_env.output_list);
        if (output) {
            audio_decoder_output_remove(output);
        }
    } while(output);

#if AUDIO_DECODER_USER_DRAM == 0
    vPortFree(audio_decoder_env.pcm);
#endif
}
