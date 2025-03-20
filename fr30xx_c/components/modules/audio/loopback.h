#ifndef _LOOPBACK_H
#define _LOOPBACK_H

#include "audio_scene.h"
#include "audio_common.h"

typedef struct {    
    uint8_t channels;
    uint32_t sample_rate;
    audio_hw_type_t in_hw_type;
    uint32_t in_hw_base_addr;
    audio_hw_type_t out_hw_type;
    uint32_t out_hw_base_addr;
} loopback_param_t;

extern audio_scene_operator_t loopback_operator;

#endif  // _LOOPBACK_H
