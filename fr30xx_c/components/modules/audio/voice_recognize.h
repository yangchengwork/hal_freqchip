#ifndef _VOICE_RECOGNIZE_H
#define _VOICE_RECOGNIZE_H

#include "audio_scene.h"

typedef struct {
    audio_type_t audio_type;
    audio_hw_type_t hw_type;
    uint32_t hw_base_addr;
    uint8_t channels;
    uint32_t sample_rate;
} voice_recognize_param_t;

extern audio_scene_operator_t voice_recognize_operator;

#endif  // _VOICE_RECOGNIZE_H
