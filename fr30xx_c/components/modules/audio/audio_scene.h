#ifndef _AUDIO_SCENE_H
#define _AUDIO_SCENE_H

#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

#include "audio_common.h"
#include "audio_hw.h"
#include "audio_decoder.h"
#include "audio_encoder.h"

typedef uint32_t (*audio_scene_decoder_req_raw_cb)(uint8_t *data, uint32_t length);
typedef void (*audio_scene_tone_destroyed_cb)(void);

typedef enum {
    /*===============  Internal Usage or Common Event ==============*/
    AUDIO_SCENE_EVT_TYPE_CREATE,
    AUDIO_SCENE_EVT_TYPE_DESTROY,
    /* new encoded frame is received, such as sbc frame received from a2dp source */
    AUDIO_SCENE_EVT_TYPE_RECV_ENCODED_FRAME,
    /* decoder request more raw data */
    AUDIO_SCENE_EVT_TYPE_REQ_ENCODED_FRAME,
    /* audio hardware working in AUDIO_HW_DIR_IN mode receive new PCM data */
    AUDIO_SCENE_EVT_TYPE_HW_IN_NEW_SAMPLES,

//    AUDIO_SCENE_EVT_TYPE_TONE_ADD,
//    AUDIO_SCENE_EVT_TYPE_TONE_REMOVE,
    
    AUDIO_SCENE_EVT_YTPE_MAX,
} audio_scene_evt_type_t;

typedef struct audio_scene_operator audio_scene_operator_t;

typedef struct {
    void *env;
    void *param;
    audio_scene_operator_t *op;
} audio_scene_t;

typedef struct {
    struct co_list_hdr hdr;
    
    audio_scene_evt_type_t type;
    audio_scene_t *scene;
} audio_scene_evt_t;

/* event for AUDIO_SCENE_EVT_TYPE_REQ_ENCODED_FRAME */
typedef struct {
    audio_scene_evt_t evt;
    audio_decoder_t *decoder;
} audio_scene_evt_req_encoded_frame_t;

/* event for AUDIO_SCENE_EVT_TYPE_HW_IN_NEW_SAMPLES */
typedef struct {
    audio_scene_evt_t evt;
    uint32_t adc_new_samples;
} audio_scene_evt_hw_in_new_samples_t;

/* event for AUDIO_SCENE_EVT_TYPE_TONE_ADD */
typedef struct {
    audio_scene_evt_t evt;
    audio_type_t audio_type;
    audio_scene_decoder_req_raw_cb cb;
} audio_scene_evt_tone_add_t;

typedef struct {
    audio_scene_evt_t evt;
    audio_data_element_t *raw_frame;
} audio_scene_evt_recv_encoded_data_t;

struct audio_scene_operator {
    audio_scene_t *(*allocate)(void *param);
    void (*init)(audio_scene_t *);
    void (*destroy)(audio_scene_t *);
    void (*event_handler)(audio_scene_t *, audio_scene_evt_t *evt);
    bool (*decoder_started)(audio_scene_t *);
    
    bool support_tone;
};

typedef struct {
    audio_type_t audio_type;
    audio_decoder_param_t decoder_param;
    
    audio_hw_type_t hw_type;
    uint32_t hw_base_addr;
    uint8_t channels;
    uint32_t sample_rate;
    audio_scene_decoder_req_raw_cb req_raw_cb;
    audio_scene_tone_destroyed_cb tone_destroyed_cb;
} audio_tone_param_t;

/*
 * @fn          audio_scene_init
 *
 * @brief       Create a FreeRTOS task of audio scene to process event.
 *
 * @param[in]   stack_size : the maximum stack size allocated for the task. @ref AUDIO_SCENE_TASK_STACK_SIZE
 * @param[in]   priority: FreeRTOS task priority.@ref AUDIO_SCENE_TASK_PRIORITY
 */
void audio_scene_init(uint32_t stack_size, uint8_t priority);

/*
 * @fn          audio_scene_create
 *
 * @brief       Function to create an audio scene by using audio_scene_send_event().
 *
 * @param[in]   op: pointer to the struct of basic opration functions of each audio scene, @ref audio_scene_operator_t.
 * @param[in]   param: pointer to the parameter structure needed in the creation of audio scene, differs depending on different audio scene.
 * 
 * @return      audio scene created.
 */
audio_scene_t *audio_scene_create(audio_scene_operator_t *op, void *param);

/*
 * @fn          audio_scene_destroy
 *
 * @brief       Function to destroy an audio scene by using audio_scene_send_event().
 *
 * @param[in]   scene : pointer to the audio scene needed to be destroyed.
 */
void audio_scene_destroy(audio_scene_t *scene);

/*
 * @fn          audio_scene_send_event
 *
 * @brief       Function to send event to the audio_scene_task to trigger practical operation of audio module.
 *
 * @param[in]   event : pointer to the event sent to the audio_scene_task.@ref audio_scene_evt_t
 */
void audio_scene_send_event(audio_scene_evt_t *event);

/*
 * @fn          audio_scene_recv_encoded_data
 *
 * @brief       Receive the encoded data from other device and transfer them to the event handler of the audio scene by audio_scene_task.
 *
 * @param[in]   scene : pointer to the audio scene receiving and processing these data.
 * @param[in]   valid: true when the data packet is intact; false indicates packet loss, different operation taken later.
 * @param[in]   buffer : pointer to the buffer stored the received data from the other side device during a HF phonecall. 
 * @param[in]   length: the length of received audio data waited to be processed 
 */
void audio_scene_recv_encoded_data(audio_scene_t *scene, bool valid, const uint8_t *buffer, uint32_t length);

/*
 * @fn          audio_scene_decoder_started
 *
 * @brief       get the state of the decoder registered to an audio scene.
 *
 * @param[in]   scene : pointer to an audio scene.
 *
 * @return      true: decoder starts; false: decoder didn't start.
 */
bool audio_scene_decoder_started(audio_scene_t *scene);

/*
 * @fn          audio_scene_tone_play
 *
 * @brief       Creat an audio scene of tone, different operation taken depends on current main scene.
 *
 * @param[in]   param : pointer to the struct of parameter needed by audio scene of tone. @ref audio_tone_param_t
 *             
 * @return      audio scene of tone created.
 */
audio_scene_t *audio_scene_tone_play(audio_tone_param_t *param);

/*
 * @fn          audio_scene_tone_stop
 *
 * @brief       stop and destroy the audio tone scene.
 * 
 * @param[in]   immediate : stop the tone immediately or not.
 */
void audio_scene_tone_stop(bool immediate);

/************************************************************************************
 * @fn      audio_scene_is_tone_pcm_all_played
 *
 * @brief   to destroy tone scene when the audio is all played.
 *
 * @return  NULL.
 */
void audio_scene_is_tone_pcm_all_played(void);

#endif  // _AUDIO_SCENE_H
