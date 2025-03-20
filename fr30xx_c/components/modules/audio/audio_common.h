#ifndef _AUDIO_COMMON_H
#define _AUDIO_COMMON_H

#include <stdint.h>
#include "co_list.h"

#define AUDIO_SPECIAL_LENGTH_FOR_PLC        0xFFFFFFFF
#define AUDIO_SPECIAL_LENGTH_FOR_INPUT_OVER 0xFFFFFFFE

typedef enum {
    AUDIO_RET_OUTPUT_ALMOTE_FULL = 4,
    AUDIO_RET_INPUT_ALMOTE_FULL = 3,
    AUDIO_RET_NEED_MORE = 2,
    AUDIO_RET_PENDING = 1,
    AUDIO_RET_OK = 0,
    AUDIO_RET_FAILED = -1,
    AUDIO_RET_ERR_CREATED = -2,
    AUDIO_RET_UNACCEPTABLE_SAMPLE_RATE = -3,
    AUDIO_RET_NOT_ALLOWED = -4,
} audio_ret_t;

typedef enum {
    AUDIO_TYPE_PCM,
    AUDIO_TYPE_SBC,
    AUDIO_TYPE_MP3,
    AUDIO_TYPE_AAC,
    AUDIO_TYPE_CVSD,
    AUDIO_TYPE_MSBC,
    AUDIO_TYPE_LC3,
    AUDIO_TYPE_SBC_V2,
    AUDIO_TYPE_OPUS_V2,
} audio_type_t;

typedef enum {
    AUDIO_CHANNELS_MONO = 1,
    AUDIO_CHANNELS_STEREO = 2,
} audio_channels_t;

typedef struct {
    struct co_list_hdr hdr;
    bool valid;
    uint32_t length;
    uint32_t offset;
    uint8_t buffer[];
} audio_data_element_t;

#endif  // _AUDIO_COMMON_H
