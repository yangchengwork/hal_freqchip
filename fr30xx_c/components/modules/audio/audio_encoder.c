#include "audio_encoder.h"

#include "FreeRTOS.h"

audio_encoder_t *audio_encoder_init(audio_type_t type, uint8_t channels, uint32_t sample_rate, uint16_t frame_max_length, audio_encoder_param_t *param)
{
    audio_encoder_t *encoder;
    
    encoder = pvPortMalloc(sizeof(audio_encoder_t));
    encoder->type = type;
    encoder->channels = channels;
    encoder->sample_rate = sample_rate;
    
    switch (type) {
        case AUDIO_TYPE_SBC:
            encoder->encoder = codec_encoder_init(CODEC_ENCODER_TYPE_SBC, &param->sbc);
            break;
        case AUDIO_TYPE_MSBC:
            encoder->encoder = codec_encoder_init(CODEC_ENCODER_TYPE_MSBC, &param->msbc);
            break;
        case AUDIO_TYPE_AAC:
            encoder->encoder = codec_encoder_init(CODEC_ENCODER_TYPE_AAC, &param->aac);
            break;
        case AUDIO_TYPE_PCM:
            encoder->encoder = codec_encoder_init(CODEC_ENCODER_TYPE_PCM, NULL);
            break;
        case AUDIO_TYPE_SBC_V2:
            encoder->encoder = codec_encoder_init(CODEC_ENCODER_TYPE_SBC_V2, &param->sbc);
            break;
        case AUDIO_TYPE_OPUS_V2:
            encoder->encoder = codec_encoder_init(CODEC_ENCODER_TYPE_OPUS_V2, &param->opus_v2);
            break;
        default:
            goto __err;
    }
    encoder->resampler = NULL;
    
    encoder->frame_count = 0;
    encoder->frame_max_length = frame_max_length;
    co_list_init(&encoder->frame_list);
    encoder->frame_tmp = NULL;
    
    return encoder;
    
__err:
    vPortFree(encoder);
    return NULL;
}

void audio_encoder_destroy(audio_encoder_t *encoder)
{
    audio_encoder_frame_t *frame;

    if (encoder->frame_tmp) {
        vPortFree(encoder->frame_tmp);
    }

    do {
        frame = (void *)co_list_pop_front(&encoder->frame_list);
        if (frame) {
            vPortFree(frame);
        }
    } while (frame);
    
    codec_encoder_destroy(encoder->encoder);
    if (encoder->resampler) {
        resample_destroy(encoder->resampler);
    }
    
    vPortFree(encoder);
}

static void save_encoded_data(audio_encoder_t *encoder, const uint8_t *buffer, uint32_t length)
{
    if (encoder->frame_tmp) {
        uint32_t last_space = encoder->frame_max_length - encoder->frame_tmp->length;
        
        if (length <= last_space) {
            memcpy(&encoder->frame_tmp->data[encoder->frame_tmp->length], buffer, length);
            encoder->frame_tmp->length += length;
        }
        else {
            if(last_space > 0) {
                memcpy(&encoder->frame_tmp->data[encoder->frame_tmp->length], buffer, last_space);
                encoder->frame_tmp->length += last_space;
            }
            
            GLOBAL_INT_DISABLE();
            co_list_push_back(&encoder->frame_list, &encoder->frame_tmp->hdr);
            encoder->frame_count++;
            encoder->frame_tmp = NULL;
            GLOBAL_INT_RESTORE();
            save_encoded_data(encoder, &buffer[last_space], length-last_space);
        }
    }
    else {
        audio_encoder_frame_t *frame;

        if (encoder->frame_max_length == FRAME_MAX_LENGTH_FIT_SINGLE) {
            frame = pvPortMalloc(sizeof(audio_encoder_frame_t) + length);
        }
        else {
            frame = pvPortMalloc(sizeof(audio_encoder_frame_t) + encoder->frame_max_length);
        }
        if (frame) {
            memcpy(&frame->data[0], buffer, length);
            frame->length = length;
            GLOBAL_INT_DISABLE();
            encoder->frame_tmp = frame;
            if ((encoder->frame_max_length == FRAME_MAX_LENGTH_FIT_SINGLE)
                    || (frame->length == encoder->frame_max_length)) {
                co_list_push_back(&encoder->frame_list, &encoder->frame_tmp->hdr);
                encoder->frame_count++;
                encoder->frame_tmp = NULL;
            }
            GLOBAL_INT_RESTORE();
        }
    }
}

int audio_encoder_encode(audio_encoder_t *encoder, const uint8_t *buffer, uint32_t length, uint8_t channels, uint32_t sample_rate)
{
    if ((encoder == NULL)
        || (encoder->channels != channels)) {
        return AUDIO_RET_FAILED;
    }
        
    /* check whether resample is needed. */
    if (sample_rate == encoder->sample_rate) {
        if (encoder->resampler) {
            resample_destroy(encoder->resampler);
            encoder->resampler = NULL;
        }
    }
    else {
        if (encoder->resampler) {
            resample_destroy(encoder->resampler);
        }
        else {
            enum resample_type type = resample_get_type(sample_rate, encoder->sample_rate);
            if (type != RESAMPLE_TYPE_INVALID) {
                return AUDIO_RET_UNACCEPTABLE_SAMPLE_RATE;
            }
            encoder->resampler = resample_init(type, channels);
        }
    }
    
    while (length) {
        if (encoder->resampler) {
            uint32_t input_length = length;
            uint32_t output_length;
            uint8_t *out_buffer = NULL;
            resample_exec(encoder->resampler, buffer, &input_length, &out_buffer, &output_length);
            while (output_length) {
                uint32_t encode_input_length = output_length;
                uint32_t encode_output_length;
                uint8_t *encode_out_buffer = NULL;
                codec_encoder_encode(encoder->encoder, out_buffer, &encode_input_length, &encode_out_buffer, &encode_output_length);
                if (encode_output_length) {
                    save_encoded_data(encoder, encode_out_buffer, encode_output_length);
                }
                out_buffer += encode_input_length;
                output_length -= encode_input_length;
            }
            buffer += input_length;
            length -= input_length;
        }
        else {
            uint32_t encode_input_length = length;
            uint32_t encode_output_length = 0;
            uint8_t *encode_out_buffer = NULL;
            codec_encoder_encode(encoder->encoder, buffer, &encode_input_length, &encode_out_buffer, &encode_output_length);
            if (encode_output_length) {
//                uint8_t *ptr = encode_out_buffer;
//                for(uint32_t i=0;i<encode_output_length;)
//                {
//                    printf("%02x", ptr[i]);
//                    i++;
//                    if((i%128) == 0)
//                    {
//                        printf("\n");
//                    }
//                }
                save_encoded_data(encoder, encode_out_buffer, encode_output_length);
            }
            buffer += encode_input_length;
            length -= encode_input_length;
        }
    }

    return AUDIO_RET_OK;
}

int audio_encoder_get_frame_count(audio_encoder_t *encoder)
{
    if (encoder) {
        return encoder->frame_count;
    }
    else {
        return 0;
    }
}

audio_encoder_frame_t *audio_encoder_frame_pop(audio_encoder_t *encoder)
{
    if (encoder) {
        audio_encoder_frame_t *frame;
        GLOBAL_INT_DISABLE();
        frame = (void *)co_list_pop_front(&encoder->frame_list);
        if (frame) {
            encoder->frame_count--;
        }
        GLOBAL_INT_RESTORE();
        return frame;
    }
    
    return NULL;
}

void audio_encoder_frame_release(audio_encoder_frame_t *frame)
{
    if (frame) {
        vPortFree(frame);
    }
}
