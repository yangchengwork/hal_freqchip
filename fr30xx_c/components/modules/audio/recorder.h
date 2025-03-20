#ifndef _RECORDER_H_
#define _RECORDER_H_

#include "audio_common.h"
#include "audio_hw.h"
#include "audio_scene.h"

typedef void (*recorder_report_encoded_frame)(void *arg, uint8_t *data, uint16_t length);

typedef struct  {
    audio_type_t encoder_type;
    audio_encoder_param_t encoder_param;

    audio_hw_type_t hw_type;
    uint32_t base_addr;
    uint32_t sample_rate;
    uint8_t channels;
    
    recorder_report_encoded_frame report_cb;
    void *report_param;
} recorder_param_t;

extern audio_scene_operator_t recoder_operator;

#endif  // _RECOREDER_H_
