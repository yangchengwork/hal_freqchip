#ifndef _AUDIO_DECODER_H
#define _AUDIO_DECODER_H

#include <stdint.h>

#include "co_list.h"
#include "codec.h"
#include "resample.h"

#include "audio_common.h"

#define AUDIO_DECODER_EVENT_REQ_RAW_DATA        0x00
#define AUDIO_DECODER_EVENT_PCM_CONSUMED        0x01

typedef struct {
    struct co_list_hdr hdr;

    /* sample count in buffer */
    uint16_t samples;
    uint16_t offset;

    int16_t pcm[];
} audio_decoder_pcm_data_t;

typedef struct {
    struct co_list_hdr hdr;

    /* ====Internal Usage==== */
    /*
     * true: fill zero when no enough data is available.
     * false: only return available only.
     */
    uint8_t immediate;
    uint32_t rd_ptr;
    uint32_t missed_samples;
} audio_decoder_output_t;

typedef struct _audio_decoder_t {
    struct co_list_hdr hdr;

    uint32_t current_sample_rate;

    struct codec_decoder_handle *decoder;
    void *resample;

    /* ====Internal Usage==== */
    /* reserved for upper layer */
    void (*evt_cb)(struct _audio_decoder_t *, uint8_t event);
    /* Store decoded PCM dat */
    struct co_list pcm_list;
    /* current write pointer in Mixed pcm buffer, unit is sample */
    uint32_t wr_ptr;
    /* current working state */
    uint8_t state;
} audio_decoder_t;

typedef union {
    struct lc3_decoder_param lc3;
    struct cvsd_decoder_param cvsd;
    struct opus_decoder_param opus;
    opus_decoder_v2_param_t opus_v2;
    struct aac_decoder_param aac;
    struct pcm_decoder_param pcm;
} audio_decoder_param_t;

/************************************************************************************
 * @fn      audio_decoder_add
 *
 * @brief   Add a new decoder to created audio decoder module. More than one decoder can be
 *          added into audio decoder module, the decoded PCM data from different decoders
 *          will be mixed into one PCM stream.
 *
 * @param   type: audio type, @ref audio_type_t.
 *          param: decoder parameters, @ref audio_decoder_param_t
 *          req_dec_cb: When available PCM data is less than a certain threshold, this
 *                  function will be call to request a new decode operation.
 *
 * @return  created decoder handler, NULL will be returned when executation is failed.
 */
audio_decoder_t *audio_decoder_add(audio_type_t type, audio_decoder_param_t *param, void (*evt_cb)(audio_decoder_t *, uint8_t event));

/************************************************************************************
 * @fn      audio_decoder_remove
 *
 * @brief   Remove a created audio decoder.
 *
 * @param   decoder: decoder handler to be removed.
 */
void audio_decoder_remove(audio_decoder_t *decoder);

/************************************************************************************
 * @fn      audio_decoder_start
 *
 * @brief   After a decoder is added, call this function to start the decoder.
 *
 * @param   decoder: decoder handler to start.
 */
void audio_decoder_start(audio_decoder_t *decoder);

/************************************************************************************
 * @fn      audio_decoder_stop
 *
 * @brief   After a decoder is started, call this function to stop the decoder.
 *
 * @param   decoder: decoder handler to stop.
 */
void audio_decoder_stop(audio_decoder_t *decoder);

/************************************************************************************
 * @fn      audio_decoder_output_add
 *
 * @brief   Add an output to created audio decoder module. More than one output can be
 *          added into audio decoder module. The requester should call audio_decoder_get_pcm
 *          to read mixed PCM stream periodically. If mixed PCM stream is not fetched
 *          on time by any output, the decoder module will be blocked.
 *
 * @param   immediate: this parameter indicates the operation when no enough data is
 *                  available in mixed PCM stream. True: fill left space with zero, False:
 *                  just fill buffer with available PCM data.
 * @param   channels: fill output buffer with mono or stereo data.
 *
 * @return  created decoder output handler, NULL will be returned when executation is failed.
 */
audio_decoder_output_t *audio_decoder_output_add(uint8_t immediate, uint8_t channels);

/************************************************************************************
 * @fn      audio_decoder_output_remove
 *
 * @brief   Remove an output from created audio decoder module. 
 *
 * @param   output: decoder output handler to be removed, @ref audio_decoder_output_t.
 */
void audio_decoder_output_remove(audio_decoder_output_t *output);

/************************************************************************************
 * @fn      audio_decoder_decode
 *
 * @brief   Called by upper layer to start a new decode operation. PCM list of this decoder
 *          will be checked at the beginning of this function, mixed PCM stream will be
 *          filled with these data. If mixed PCM stream is full enough, this function will
 *          return without decoding incoming raw data. This strategy can avoid PCM list becoming
 *          too long.
 *
 * @param   decoder: decoder handler.
 * @param   buffer: origin raw data buffer.
 * @param   length: how many available data in buffer, this field will be updated with
 *                  used data length before return from this function.
 *
 * @return  PCM buffer level status of this decoder, @ref audio_ret_t.
 */
int audio_decoder_decode(audio_decoder_t *decoder, const uint8_t *buffer, uint32_t* length);

/************************************************************************************
 * @fn      audio_decoder_get_pcm
 *
 * @brief   used by output to fetch PCM data.
 *
 * @param   output: output handler.
 * @param   pcm: buffer to store PCM data.
 * @param   samples: number of request samples.
 * @param   channels: mono(1) or stereo(2), @ref audio_channels_t.
 *
 * @return  actual saved samples into buffer.
 */
uint32_t audio_decoder_get_pcm(audio_decoder_output_t *output, int16_t *pcm, uint32_t samples, uint8_t channels);

/************************************************************************************
 * @fn      audio_decoder_init
 *
 * @brief   Init audio decoder module.
 *
 * @param   channels: channel numbers stored in internal mixed PCM buffer.
 * @param   out_sample_rate: PCM sample rate stored in internal mixed PCM buffer.
 *
 * @return  init result, @ref audio_ret_t.
 */
int audio_decoder_init(uint8_t channels, uint32_t out_sample_rate);

/************************************************************************************
 * @fn      audio_decoder_destroy
 *
 * @brief   Deinit audio decoder module.
 */
void audio_decoder_destroy(void);

/************************************************************************************
 * @fn      audio_decorder_is_started
 *
 * @brief   check audio decorder is started or not.
 *
 * @param   decoder: decoder handler.
 *
 * @return  true or false.
 */
bool audio_decoder_is_started(audio_decoder_t *decoder);

/************************************************************************************
 * @fn      audio_decoder_get_missed_sample_cnt
 *
 * @brief   get missed sample cnt, used to check if decoder can offer enough pcm data.
 *
 * @param   output: output handler.
 *
 * @return  missed sample cnt.
 */
uint32_t audio_decoder_get_missed_sample_cnt(audio_decoder_output_t *output);

/************************************************************************************
 * @fn      audio_decoder_pause
 *
 * @brief   pause audio decorder cnt untill buffered enough raw data.
 *
 * @param   output: output handler.
 *
 * @return  NULL.
 */
void audio_decoder_clear_missed_sample_cnt(audio_decoder_output_t *output);


#endif  //_AUDIO_DECODER_H

