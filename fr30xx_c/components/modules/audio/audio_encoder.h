#ifndef _AUDIO_ENCODER_H
#define _AUDIO_ENCODER_H

#include <stdint.h>

#include "co_list.h"
#include "codec.h"
#include "resample.h"

#include "audio_common.h"

/*
 * when frame_max_length is configured to FRAME_MAX_LENGTH_FIT_SINGLE when calling
 * function audio_encoder_init. audio_encoder_frame_t will only save one encoded frame.
 * This configuration will be useful when the length of encoded frame is vairable.
 */
#define FRAME_MAX_LENGTH_FIT_SINGLE         0xFFFF

typedef struct {
    struct co_list_hdr hdr;
    
    uint16_t length;
    uint8_t data[];
} audio_encoder_frame_t;

typedef union {
    struct sbc_encoder_param sbc;
    struct aac_encoder_param aac;
    struct msbc_encoder_param msbc;
    struct lc3_encoder_param lc3;
    struct cvsd_encoder_param cvsd;
    struct opus_encoder_param opus;
    opus_encoder_v2_param_t opus_v2;
} audio_encoder_param_t;

typedef struct {
    audio_type_t type;
    uint8_t channels;
    uint32_t sample_rate;
    
    void *encoder;
    void *resampler;
    
    uint8_t frame_count;
    uint16_t frame_max_length;
    struct co_list frame_list;
    audio_encoder_frame_t *frame_tmp;
} audio_encoder_t;

/************************************************************************************
 * @fn      audio_encoder_init
 *
 * @brief   Init audio encoder module.
 *
 * @param   type: audio encoder type.
 *          channels: mono or stereo.
 *          sample_rate: input PCM sample rate.
 *          frame_max_length: 
 *          param: encoder parameter.
 *
 * @return  audio encoder handler, NULL will be returned when executation is failed.
 */
audio_encoder_t *audio_encoder_init(audio_type_t type, uint8_t channels, uint32_t sample_rate, uint16_t frame_max_length, audio_encoder_param_t *param);

/************************************************************************************
 * @fn      audio_encoder_destroy
 *
 * @brief   deinit audio encoder module.
 *
 * @param   encoder: encoder handler.
 */
void audio_encoder_destroy(audio_encoder_t *encoder);

/************************************************************************************
 * @fn      audio_encoder_encode
 *
 * @brief   encode input PCM data.
 *
 * @param   encoder: encoder handler.
 * @param   buffer: PCM buffer.
 * @param   length: size of input PCM data, unit is bytes.
 * @param   channels: mono or stereo of input PCM data
 * @param   sample_rate: sample rate of input PCM data.
 *
 * @return  encoder result, @ref audio_ret_t.
 */
int audio_encoder_encode(audio_encoder_t *encoder, const uint8_t *buffer, uint32_t length, uint8_t channels, uint32_t sample_rate);

/************************************************************************************
 * @fn      audio_encoder_get_frame_count
 *
 * @brief   Get the number of frames are stored in encoder module.
 *
 * @param   encoder: encoder handler.
 *
 * @return  the number of frames are stored in encoder module.
 */
int audio_encoder_get_frame_count(audio_encoder_t *encoder);

/************************************************************************************
 * @fn      audio_encoder_frame_pop
 *
 * @brief   Get the head of frames stored in encoder module.
 *
 * @param   encoder: encoder handler.
 *
 * @return  the head of frames stored in encoder module.
 */
audio_encoder_frame_t *audio_encoder_frame_pop(audio_encoder_t *encoder);

/************************************************************************************
 * @fn      audio_encoder_frame_release
 *
 * @brief   Release encoded frame.
 *
 * @param   frame: frame to be released.
 */
void audio_encoder_frame_release(audio_encoder_frame_t *frame);

#endif  //_AUDIO_ENCODER_H
