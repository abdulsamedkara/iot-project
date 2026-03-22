#pragma once
#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

esp_err_t voice_client_transcribe(const uint8_t *pcm_data, size_t pcm_len,
                                   const char *event_hdr, const char *state_hdr,
                                   uint8_t *resp_buf, size_t resp_buf_sz,
                                   size_t *resp_len, char *action_out,
                                   char *transcript_out, size_t transcript_sz,
                                   char *answer_out, size_t answer_sz);

esp_err_t voice_client_sensor_update(float temp, float light, float noise,
                                     uint8_t *resp_buf, size_t resp_buf_sz,
                                     size_t *resp_len);
