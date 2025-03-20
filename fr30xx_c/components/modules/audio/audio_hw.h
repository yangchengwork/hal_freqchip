#ifndef _AUDIO_HW_H
#define _AUDIO_HW_H

#include <stdint.h>

#include "co_list.h"

typedef void (*audio_hw_request_pcm_t)(void *pcm, uint32_t samples, uint8_t channels);
typedef void (*audio_hw_receive_pcm_ntf_t)(uint32_t samples);

typedef enum {
    AUDIO_HW_TYPE_I2S,
    AUDIO_HW_TYPE_CODEC,
    AUDIO_HW_TYPE_PDM,
    AUDIO_HW_TYPE_SPDIF,
    AUDIO_HW_TYPE_PSD_DAC,
} audio_hw_type_t;

typedef enum {
    AUDIO_HW_DIR_IN = 0x01,
    AUDIO_HW_DIR_OUT = 0x02,
    AUDIO_HW_DIR_INOUT = (AUDIO_HW_DIR_IN | AUDIO_HW_DIR_OUT),
} audio_hw_dir_t;

typedef struct {
    struct co_list_hdr hdr;

    audio_hw_type_t type;
    audio_hw_dir_t dir;
    uint8_t channels;
    uint32_t sample_rate;
    uint32_t base_addr;
    void *hw_handle;

    /* used for output mode */
    audio_hw_request_pcm_t request_handler;
    int16_t *pcm_out;

    /* used for input mode */
    uint32_t wr_ptr;        /* unit is sample */
    struct co_list output_list;
    uint32_t pcm_samples;   /* unit is sample */
    uint8_t *pcm;
} audio_hw_t;

typedef struct {
    struct co_list_hdr hdr;
    
    audio_hw_t *audio_hw;
    audio_hw_receive_pcm_ntf_t handler;
    uint32_t rd_ptr;    /* unit is sample */
} audio_hw_output_t;

/*
 * @fn          audio_hw_create
 *
 * @brief       Creat an audio hardware instance. config the hardware and enable the interrupt. CODEC,I2S,PDM supported.
 *
 * @param[in]   type: hardware type @ref audio_hw_type_t.
 * @param[in]   handler: handler for hw to request pcm data from decoder pcm buffer, not necessary
 * @param[in]   base_addr: hardware base address, when the type is CODEC, it's invalid. 
 * @param[in]   dir: the direction from other module to hardware. @ref audio_hw_dir_t.
 * @param[in]   sample_rate: the sample rate of audio flow. 
 * @param[in]   channels: channel number of audio flow. 
 * 
 * @return      audio hw instance created .
 */
audio_hw_t *audio_hw_create(audio_hw_type_t type, audio_hw_request_pcm_t handler, uint32_t base_addr, audio_hw_dir_t dir, uint32_t sample_rate, uint8_t channels);

/*
 * @fn          audio_hw_destroy
 *
 * @brief       destroy an audio hw instance.
 *
 * @param[in]   hw : an audio hardware instance.
 */
void audio_hw_destroy(audio_hw_t *hw);

/*
 * @fn          audio_hw_output_add
 *
 * @brief       Add an output to the output list of an audio hardware instance.
 *
 * @param[in]   hw: an audio hardware instance.
 * @param[in]   handler: callback handler to notify the next module to receive the date from the hardware output.
 *              
 * @return      the initialized audio hardware output structure.
 */
audio_hw_output_t *audio_hw_output_add(audio_hw_t *hw, audio_hw_receive_pcm_ntf_t handler);

/*
 * @fn          audio_hw_output_remove
 *
 * @brief       remove an output from the output list of an audio hardware instance.
 *
 * @param[in]   hw: an audio hardware instance.
 * @param[in]   output: the output waited to be removed.
 */
void audio_hw_output_remove(audio_hw_t *hw, audio_hw_output_t *output);

/*
 * @fn          audio_hw_read_pcm
 *
 * @brief       read pcm data from hardware.
 *
 * @param[in]   output: pointer to the output structure added to hardware output list before.
 * @param[in]   pcm: pointer to the buffer to stored the data read from hardware.
 * @param[in]   samples: data number wanted to be read from hw. 
 * @param[in]   channels: channel number of PCM data wanted to be read from hardware. 
 * 
 * @return      data number read from hardware successfully.
 */
uint32_t audio_hw_read_pcm(audio_hw_output_t *output, void *pcm, uint32_t samples, uint8_t channels);

#endif  // _AUDIO_HW_H
