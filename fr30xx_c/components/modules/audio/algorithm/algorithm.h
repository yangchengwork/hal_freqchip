#ifndef _ALGORITHM_H
#define _ALGORITHM_H

#include <stdint.h>

enum audio_algo_sel {
    AUDIO_ALGO_SEL_AEC  = 0x01,     //Acoustic Echo Cancellation
    AUDIO_ALGO_SEL_NS   = 0x02,     //noise suppression
    AUDIO_ALGO_SEL_AGC  = 0x04,     //Automatic Gain Control
};

/************************************************************************************
 * @fn      algorithm_init
 *
 * @brief   init an audio algorithm instance.
 *
 * @param   algo_sel: algorithm select, @ref enum audio_algo_sel.
 * @param   sample_rate: sample rate of PCM data.
 * @param   ns_level: noise suppression level.
 * @param   agc_mode: Automatic Gain Control level.
 * @param   frame_size: algo frame size, unit is sample.
 *
 * @return  handler of created audio algorithm instance.
 */
void *algorithm_init(uint8_t algo_sel, uint32_t sample_rate, uint8_t ns_level, uint16_t agc_mode, uint32_t *frame_size);

/************************************************************************************
 * @fn      algorithm_launch
 *
 * @brief   request start algorithim
 *
 * @param   handle: audio algo handler.
 * @param   farend: pointer to the audio data from the other side .
 * @param   nearend: audio data to be cancel echos.
 * @param   out: output stored processed data.
 *
 * @return  0: start succeeded; -1: failed
 */
int algorithm_launch(void *handle, int16_t *farend, int16_t *nearend, int16_t **out);

/************************************************************************************
 * @fn      algorithm_destroy
 *
 * @brief   destroy an audio algo instance.
 *
 * @param   handle: audio algo handler.
 *
 * @return  NULL.
 */
void algorithm_destroy(void *handle);

#endif  // _ALGORITHM_H
