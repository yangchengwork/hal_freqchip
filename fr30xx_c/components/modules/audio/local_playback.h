#ifndef _LOCAL_PLAYBACK_H
#define _LOCAL_PLAYBACK_H

#include "audio_scene.h"
#include "audio_common.h"

typedef struct {
    audio_type_t audio_type;
    audio_decoder_param_t decoder_param;
    
    uint8_t channels;
    uint16_t sample_rate;
    audio_hw_type_t hw_type;
    uint32_t hw_base_addr;
    
    audio_scene_decoder_req_raw_cb req_raw_cb;
} local_playback_param_t;

extern audio_scene_operator_t audio_local_playback_operator;

#endif  // _LOCAL_PLAYBACK_H
