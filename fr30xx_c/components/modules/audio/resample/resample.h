#ifndef _RESAMPLE_H
#define _RESAMPLE_H

#include <stdint.h>

enum resample_type {
    RESAMPLE_TYPE_D_48000_44100,
    RESAMPLE_TYPE_D_64000_8000,
    RESAMPLE_TYPE_D_64000_32000,
    RESAMPLE_TYPE_D_32000_16000,
    RESAMPLE_TYPE_D_16000_8000,
    RESAMPLE_TYPE_D_44100_16000,

    RESAMPLE_TYPE_U_8000_16000,
    RESAMPLE_TYPE_U_8000_64000,
    RESAMPLE_TYPE_U_8000_16000_CVSD,
    RESAMPLE_TYPE_U_16000_32000,
    RESAMPLE_TYPE_U_32000_64000,

    RESAMPLE_TYPE_D_48000_16000,
    RESAMPLE_TYPE_INVALID,
};

/************************************************************************************
 * @fn      resample_init
 *
 * @brief   init resample instance.
 *
 * @param   type: type of resample operation, from a samplerate to another. @ref enum resample_type.
 * @param   channels: channels of data to be resampled.
 *
 * @return  handler of created resample instance.
 */
void *resample_init(enum resample_type type, uint8_t channels);

/************************************************************************************
 * @fn      resample_destroy
 *
 * @brief   destroy a resample instance.
 *
 * @param   handle: resample handler.
 *
 * @return  NULL.
 */
void resample_destroy(void *handle);

/************************************************************************************
 * @fn      resample_exec
 *
 * @brief   request to execute resample
 *
 * @param   handle: resample handler.
 * @param   indata: pointer to the audio data to be resample.
 * @param   insize: length of data to be resample.
 * @param   out_buf: output stored resampled data.
 * @param   out_length: length of data resampled.
 *
 * @return  result of resample execution, -1 means false.
 */
int resample_exec(void *handle, const uint8_t *indata, uint32_t *insize, uint8_t **out_buf, uint32_t *out_length);

/************************************************************************************
 * @fn      resample_get_type
 *
 * @brief   get the type of resample operation.
 *
 * @param   in_sample_rate: original sample rate of PCM data.
 * @param   out_sample_rate: output PCM sample rate set in decoder module environment during initialization.
 *
 * @return  type of resample operation,  @ref enum resample_type.
 */
enum resample_type resample_get_type(uint32_t in_sample_rate, uint32_t out_sample_rate);

#endif  // _RESAMPLE_H

