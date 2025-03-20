/*
 * codec.h
 *
 *  Created on: 2018-3-28
 *      Author: Administrator
 */

#ifndef _CODEC_H
#define _CODEC_H

#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"   // for malloc

#define codec_alloc          pvPortMalloc
#define codec_free           vPortFree

enum codec_error {
    CODEC_ERROR_NO_ERROR,
    CODEC_ERROR_FAILED,
    CODEC_ERROR_INVALID_HANDLE,
    CODEC_ERROR_UNACCEPTABLE_PARAM,
    CODEC_ERROR_INSUFFICIENT_RESOURCE,
    CODEC_ERROR_NONE_FATAL,
    CODEC_ERROR_FATAL,
    CODEC_ERROR_NEED_MORE_DATA,
};

enum codec_decoder_type {
    CODEC_DECODER_TYPE_MP3,
    CODEC_DECODER_TYPE_CVSD,
    CODEC_DECODER_TYPE_LC3,
    CODEC_DECODER_TYPE_MSBC,
    CODEC_DECODER_TYPE_SBC,
    CODEC_DECODER_TYPE_OGGOPUS,
    CODEC_DECODER_TYPE_AAC,
    CODEC_DECODER_TYPE_PCM,
    CODEC_DECODER_TYPE_SBC_V2,
    CODEC_DECODER_TYPE_OPUS_V2,
};

enum codec_encoder_type {
    CODEC_ENCODER_TYPE_CVSD,
    CODEC_ENCODER_TYPE_LC3,
    CODEC_ENCODER_TYPE_MSBC,
    CODEC_ENCODER_TYPE_SBC,
    CODEC_ENCODER_TYPE_OPUS,
    CODEC_ENCODER_TYPE_AAC,
    CODEC_ENCODER_TYPE_PCM,
    CODEC_ENCODER_TYPE_SBC_V2,
    CODEC_ENCODER_TYPE_OPUS_V2,
};

struct sbc_encoder_param {
    uint32_t i_samp_freq;
    uint32_t i_num_chan;
    uint32_t i_subbands;
    uint32_t i_blocks;
    uint32_t i_bitpool;
    uint32_t i_snr;
};

struct aac_encoder_param {
    uint32_t i_samp_freq;
    uint32_t i_num_chan;
    uint32_t i_pcm_wdsz;
};

struct msbc_encoder_param {
    uint32_t i_bitrate;
    uint32_t i_samp_freq;
};

struct lc3_encoder_param {
    int dt_ms;
    int sr_hz;
    int bit_rate;
    int bips_in;
    int ch;
};

struct lc3_decoder_param{
    int bips_out;
    uint32_t sample_rate;
    int16_t nchannels;
    int bitrate;
    float frame_ms;
    uint32_t signal_len;
    int epmode;
    int hrmode;
    uint16_t frame_size;
};

struct cvsd_encoder_param {
    int ch;
    double step_decay; //I2 decay
    double accum_decay; //I1 decay
};

struct cvsd_decoder_param{
    int ch;

    double step_decay; //I2 decay
    double accum_decay; //I1 decay
};

struct opus_decoder_param{
    int sample_rate;
    int nb_coupled;
    int channel_mapping;
    uint8_t channels;
    uint8_t frames_per_pack;
    int gain;
};

struct opus_encoder_param{
    int application;
    int sampleRate;
    int numChannels;
    int bitRate;
    float frame_size_ms;
};

typedef struct {
    uint32_t sample_rate;
    uint32_t frame_size;
    uint32_t bitrate;
    uint8_t complexity;
    uint8_t channels;
} opus_encoder_v2_param_t;

typedef struct {
    uint32_t sample_rate;
    uint32_t frame_size;
    uint8_t channels;
} opus_decoder_v2_param_t;

struct aac_decoder_param{
    int PcmWidth;
};

struct pcm_decoder_param {
    uint32_t sample_rate;
    uint16_t frame_size;
    uint8_t channels;
};

struct codec_decoder_api {
    void *(*init)(void *param);
    void (*destroy)(void *handle);
    int (*decode)(void *handle, const uint8_t *data, uint32_t *length, uint8_t **out_buf, uint32_t *out_length);
    int (*input_over)(void *handle, uint8_t **out_buf, uint32_t *out_length, uint8_t *exec_done);
    int (*plc)(void *handle, uint8_t **out_buf, uint32_t *out_length);
    int (*get_param)(void *handle, uint32_t *sample_rate, uint8_t *channels);
};

struct codec_decoder_handle {
	struct codec_decoder_api *api;
	void *decoder_env;
};

struct codec_encoder_api {
    void *(*init)(void *param);
    void (*destroy)(void *handle);
    int (*encode)(void *handle, const uint8_t *data, uint32_t *length, uint8_t **out_buf, uint32_t *out_length);
};

struct codec_encoder_handle {
	struct codec_encoder_api *api;
	void *encoder_env;
};

struct codec_decoder_handle *codec_decoder_init(uint8_t decoder_type, void *param);
void codec_decoder_destroy(struct codec_decoder_handle *handle);
int codec_decoder_decode(struct codec_decoder_handle *handle,
                            const uint8_t *in_buf,
                            uint32_t *in_length,
                            uint8_t **out_buf,
                            uint32_t *out_length);
int codec_decoder_plc(struct codec_decoder_handle *handle,
                            uint8_t **out_buf,
                            uint32_t *out_length);
int codec_decoder_input_over(struct codec_decoder_handle *handle,
                            uint8_t **out_buf,
                            uint32_t *out_length,
                            uint8_t *exec_done);
int codec_decoder_get_param(struct codec_decoder_handle *handle, uint32_t *sample_rate, uint8_t *channels);

struct codec_encoder_handle *codec_encoder_init(uint8_t encoder_type, void *param);
void codec_encoder_destroy(struct codec_encoder_handle *handle);
int codec_encoder_encode(struct codec_encoder_handle *handle,
                            const uint8_t *in_buf,
                            uint32_t *in_length,
                            uint8_t **out_buf,
                            uint32_t *out_length);
int codec_encoder_plc(struct codec_encoder_handle *handle,
                            uint8_t **out_buf,
                            uint32_t *out_length);
#endif /* _CODEC_H */
