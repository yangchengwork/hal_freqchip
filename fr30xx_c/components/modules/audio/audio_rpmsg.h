#ifndef _AUDIO_RPMSG_H
#define _AUDIO_RPMSG_H

#include <stdint.h>

#include "rpmsg.h"

#define RPMSG_SYNC_FUNC_DEC_INIT                    RPMSG_SYNC_FUNC_MSG(RPMSG_SYNC_FUNC_TYPE_AUDIO, 0x0001)
#define RPMSG_SYNC_FUNC_DEC_DESTROY                 RPMSG_SYNC_FUNC_MSG(RPMSG_SYNC_FUNC_TYPE_AUDIO, 0x0002)
#define RPMSG_SYNC_FUNC_DEC_EXEC                    RPMSG_SYNC_FUNC_MSG(RPMSG_SYNC_FUNC_TYPE_AUDIO, 0x0003)
#define RPMSG_SYNC_FUNC_DEC_PLC                     RPMSG_SYNC_FUNC_MSG(RPMSG_SYNC_FUNC_TYPE_AUDIO, 0x0004)
#define RPMSG_SYNC_FUNC_DEC_GET_PARAM               RPMSG_SYNC_FUNC_MSG(RPMSG_SYNC_FUNC_TYPE_AUDIO, 0x0005)
#define RPMSG_SYNC_FUNC_ENC_INIT                    RPMSG_SYNC_FUNC_MSG(RPMSG_SYNC_FUNC_TYPE_AUDIO, 0x0006)
#define RPMSG_SYNC_FUNC_ENC_DESTROY                 RPMSG_SYNC_FUNC_MSG(RPMSG_SYNC_FUNC_TYPE_AUDIO, 0x0007)
#define RPMSG_SYNC_FUNC_ENC_EXEC                    RPMSG_SYNC_FUNC_MSG(RPMSG_SYNC_FUNC_TYPE_AUDIO, 0x0008)
#define RPMSG_SYNC_FUNC_RESAMPLE_INIT               RPMSG_SYNC_FUNC_MSG(RPMSG_SYNC_FUNC_TYPE_AUDIO, 0x0009)
#define RPMSG_SYNC_FUNC_RESAMPLE_EXEC               RPMSG_SYNC_FUNC_MSG(RPMSG_SYNC_FUNC_TYPE_AUDIO, 0x000a)
#define RPMSG_SYNC_FUNC_RESAMPLE_DESTROY            RPMSG_SYNC_FUNC_MSG(RPMSG_SYNC_FUNC_TYPE_AUDIO, 0x000b)
#define RPMSG_SYNC_FUNC_AUDIO_ALGO_INIT             RPMSG_SYNC_FUNC_MSG(RPMSG_SYNC_FUNC_TYPE_AUDIO, 0x000c)
#define RPMSG_SYNC_FUNC_AUDIO_ALGO_LAUNCH           RPMSG_SYNC_FUNC_MSG(RPMSG_SYNC_FUNC_TYPE_AUDIO, 0x000d)
#define RPMSG_SYNC_FUNC_AUDIO_ALGO_RELEASE          RPMSG_SYNC_FUNC_MSG(RPMSG_SYNC_FUNC_TYPE_AUDIO, 0x000e)
#define RPMSG_SYNC_FUNC_AUDIO_FT                    RPMSG_SYNC_FUNC_MSG(RPMSG_SYNC_FUNC_TYPE_AUDIO, 0x000f)
#define RPMSG_SYNC_FUNC_VOICE_RECOGNIZE_INIT        RPMSG_SYNC_FUNC_MSG(RPMSG_SYNC_FUNC_TYPE_AUDIO, 0x0010)
#define RPMSG_SYNC_FUNC_VOICE_RECOGNIZE_LAUNCH      RPMSG_SYNC_FUNC_MSG(RPMSG_SYNC_FUNC_TYPE_AUDIO, 0x0011)
#define RPMSG_SYNC_FUNC_VOICE_RECOGNIZE_RELEASE     RPMSG_SYNC_FUNC_MSG(RPMSG_SYNC_FUNC_TYPE_AUDIO, 0x0012)
#define RPMSG_SYNC_FUNC_DEC_INPUT_DONE              RPMSG_SYNC_FUNC_MSG(RPMSG_SYNC_FUNC_TYPE_AUDIO, 0x0013)

struct rpmsg_sync_msg_decoder_init_t {
    uint8_t decoder_type;
    void *param;
};

struct rpmsg_sync_msg_decoder_exec_t {
    void *handle;
    const uint8_t *in_buffer;
    uint32_t *in_length;
    uint8_t **out_buffer;
    uint32_t *out_length;
};

struct rpmsg_sync_msg_decoder_plc_t {
    void *handle;
    uint8_t **out_buffer;
    uint32_t *out_length;
};

struct rpmsg_sync_msg_decoder_input_over_t {
    void *handle;
    uint8_t **out_buffer;
    uint32_t *out_length;
    uint8_t *exec_done;
};

struct rpmsg_sync_msg_decoder_get_param_t {
    void *handle;
    uint32_t *sample_rate;
    uint8_t *channels;
};

struct rpmsg_sync_msg_decoder_destroy_t {
    void *handle;
};

struct rpmsg_sync_msg_encoder_init_t {
    uint8_t encoder_type;
    void *param;
};

struct rpmsg_sync_msg_encoder_exec_t {
    void *handle;
    const uint8_t *in_buffer;
    uint32_t *in_length;
    uint8_t **out_buffer;
    uint32_t *out_length;
};

struct rpmsg_sync_msg_encoder_destroy_t {
    void *handle;
};

struct rpmsg_sync_msg_resample_init_t {
    uint8_t resample_type;
    uint8_t channels;
};

struct rpmsg_sync_msg_resample_exec_t {
    void *handle;
    const uint8_t *in_buffer;
    uint32_t *in_length;
    uint8_t **out_buffer;
    uint32_t *out_length;
};

struct rpmsg_sync_msg_resample_destroy_t {
    void *handle;
};

struct rpmsg_sync_msg_algorithm_init {
    uint8_t algo_sel;
    uint32_t sample_rate;
    uint8_t ns_level;
    uint16_t agc_mode;
    uint32_t *frame_size;
};

struct rpmsg_sync_msg_algorithm_launch {
    void *handle;
    const int16_t *farend;
    const int16_t *nearend;
    int16_t **out;
};

struct rpmsg_sync_msg_algorithm_destroy_t {
    void *handle;
};

struct rpmsg_sync_msg_voice_recognize_launch_t {
    const int16_t *mic_pcm;
    uint32_t samples;
};

#endif  // _AUDIO_RPMSG_H

