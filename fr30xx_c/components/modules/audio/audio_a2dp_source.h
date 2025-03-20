#ifndef _AUDIO_A2DP_SOURCE_H_
#define _AUDIO_A2DP_SOURCE_H_

#include "audio_common.h"
#include "audio_scene.h"

typedef void (*audio_a2dp_source_report_encoded_frame)(void *arg, uint8_t *data, uint16_t length);

typedef struct {
     /* Bitpool affects the bitrate of the stream according to the following 
     * formula: bit_rate = 8 * frameLength * sampleFreq / numSubBands / 
     * numBlocks.  The frameLength value can be determined by setting the 
     * bitPool value and calling the SBC_FrameLen() function.  
     * Bitpool can be changed dynamically from frame to frame during 
     * encode/decode without suspending the stream.  
     */   
    uint8_t   bitpool;

    /* Sampling frequency of the stream */ 
    uint16_t sample_rate;

    /* The allocation method for the stream */ 
    uint8_t alloc_method;
}a2dp_sbc_param_t;

typedef struct {
    uint8_t channels;
    uint32_t sample_rate;
    
    audio_type_t audio_input_type;
    audio_decoder_param_t decoder_param;
    
    audio_type_t audio_output_type;
    audio_encoder_param_t encoder_param;
    
    audio_hw_type_t hw_type;
    uint32_t hw_base_addr;
    
    audio_a2dp_source_report_encoded_frame report_enc_cb;
    void *report_enc_arg;
    audio_scene_decoder_req_raw_cb dec_req_raw_cb;
} audio_a2dp_source_param_t;

extern audio_scene_operator_t audio_a2dp_source_operator;

#endif  // _AUDIO_A2DP_SOURCE_H_
