#ifndef _AUDIO_A2DP_SINK_H_
#define _AUDIO_A2DP_SINK_H_

#include "audio_common.h"
#include "audio_hw.h"
#include "audio_scene.h"

typedef struct  {
    audio_type_t decoder_type;
    audio_decoder_param_t decoder_param;
    
    audio_hw_type_t hw_type;
    uint32_t hw_base_addr;
    uint32_t sample_rate;
    uint8_t channels;
} audio_a2dp_sink_param_t;

extern audio_scene_operator_t audio_a2dp_sink_operator;

#endif  // _AUDIO_A2DP_SINK_H_
