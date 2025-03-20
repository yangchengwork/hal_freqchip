#ifndef _AUDIO_SCO_H_
#define _AUDIO_SCO_H_

#include "audio_common.h"
#include "audio_scene.h"

typedef void (*audio_sco_report_encoded_frame)(void *arg, uint8_t *data, uint16_t length);

typedef struct {
    audio_type_t audio_type;
    audio_decoder_param_t decoder_param;
    audio_encoder_param_t encoder_param;
    
    audio_hw_type_t hw_type;
    uint32_t sample_rate;
    uint32_t hw_base_addr;
    // void (*req_dec_cb)(audio_decoder_t *);
    audio_sco_report_encoded_frame report_enc_cb;
    void *report_enc_arg;
} audio_sco_param_t;

extern audio_scene_operator_t audio_sco_operator;

#endif  // _AUDIO_SCO_H_
