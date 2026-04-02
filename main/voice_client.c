#include "voice_client.h"
#include "config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "voice_client";

static void url_decode(char *dst, const char *src, size_t dst_size) {
    size_t d = 0;
    while (*src && d < dst_size - 1) {
        if (*src == '%') {
            if (src[1] && src[2]) {
                int v;
                sscanf(src + 1, "%2x", &v);
                dst[d++] = (char)v;
                src += 3;
            } else { break; }
        } else if (*src == '+') {
            dst[d++] = ' ';
            src++;
        } else {
            dst[d++] = *src++;
        }
    }
    dst[d] = '\0';
}

typedef struct {
    uint8_t *buf;
    size_t   buf_sz;
    size_t   len;
    bool     overflow;
    char    *action_out;
    char    *transcript_out;
    size_t   transcript_sz;
    char    *answer_out;
    size_t   answer_sz;
} resp_ctx_t;

static esp_err_t http_evt(esp_http_client_event_t *evt)
{
    resp_ctx_t *ctx = (resp_ctx_t *)evt->user_data;
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        if (!ctx->overflow && ctx->len + evt->data_len <= ctx->buf_sz) {
            memcpy(ctx->buf + ctx->len, evt->data, evt->data_len);
            ctx->len += evt->data_len;
        } else {
            ctx->overflow = true;
            ESP_LOGW(TAG, "Yanıt tamponu doldu!");
        }
    } else if (evt->event_id == HTTP_EVENT_ON_HEADER) {
        if (strcasecmp(evt->header_key, "X-Action") == 0 && ctx->action_out) {
            strncpy(ctx->action_out, evt->header_value, 31);
            ctx->action_out[31] = '\0';
            ESP_LOGI(TAG, "Gelen X-Action: %s", ctx->action_out);
        } else if (strcasecmp(evt->header_key, "X-Transcript-URL") == 0 && ctx->transcript_out) {
            url_decode(ctx->transcript_out, evt->header_value, ctx->transcript_sz);
            ESP_LOGI(TAG, "Gelen Transcript: %s", ctx->transcript_out);
        } else if (strcasecmp(evt->header_key, "X-Answer-URL") == 0 && ctx->answer_out) {
            url_decode(ctx->answer_out, evt->header_value, ctx->answer_sz);
            ESP_LOGI(TAG, "Gelen Answer: %s", ctx->answer_out);
        }
    } else if (evt->event_id == HTTP_EVENT_ERROR) {
        ESP_LOGE(TAG, "HTTP hatası");
    }
    return ESP_OK;
}

esp_err_t voice_client_transcribe(const uint8_t *pcm_data, size_t pcm_len,
                                   const char *event_hdr, const char *state_hdr,
                                   uint8_t *resp_buf, size_t resp_buf_sz,
                                   size_t *resp_len, char *action_out,
                                   char *transcript_out, size_t transcript_sz,
                                   char *answer_out, size_t answer_sz)
{
    char url[128];
    snprintf(url, sizeof(url), "http://%s:%d/transcribe", SERVER_HOST, SERVER_PORT);

    if (action_out) action_out[0] = '\0';
    if (transcript_out) transcript_out[0] = '\0';
    if (answer_out) answer_out[0] = '\0';
    
    resp_ctx_t ctx = { 
        .buf = resp_buf, .buf_sz = resp_buf_sz, .len = 0, .overflow = false, 
        .action_out = action_out,
        .transcript_out = transcript_out, .transcript_sz = transcript_sz,
        .answer_out = answer_out, .answer_sz = answer_sz
    };

    esp_http_client_config_t cfg = {
        .url            = url,
        .method         = HTTP_METHOD_POST,
        .event_handler  = http_evt,
        .user_data      = &ctx,
        .timeout_ms     = 180000,  /* LLM ve TTS için 3 dakika bekleme */
        .buffer_size    = 4096,
        .buffer_size_tx = 4096,
        .keep_alive_enable = true,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) return ESP_FAIL;

    esp_http_client_set_header(client, "Content-Type", "application/octet-stream");
    if (event_hdr) {
        esp_http_client_set_header(client, "X-Event", event_hdr);
    }
    if (state_hdr) {
        esp_http_client_set_header(client, "X-Pomo-State", state_hdr);
    }
    
    if (pcm_data && pcm_len > 0) {
        esp_http_client_set_post_field(client, (const char *)pcm_data, (int)pcm_len);
    } else {
        esp_http_client_set_post_field(client, "", 0);
    }

    ESP_LOGI(TAG, "Gönderiliyor: %zu bayt PCM → %s", pcm_len, url);
    esp_err_t ret = esp_http_client_perform(client);

    if (ret == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        if (status == 200 && !ctx.overflow) {
            *resp_len = ctx.len;
            ESP_LOGI(TAG, "Yanıt: %zu bayt WAV", ctx.len);
        } else {
            ESP_LOGE(TAG, "Sunucu hatası: HTTP %d", status);
            ret = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "HTTP başarısız: %s", esp_err_to_name(ret));
    }

    esp_http_client_cleanup(client);
    return ret;
}

esp_err_t voice_client_sensor_update(float temp, float light, float noise,
                                     uint8_t *resp_buf, size_t resp_buf_sz,
                                     size_t *resp_len)
{
    char url[128];
    snprintf(url, sizeof(url), "http://%s:%d/sensor_update", SERVER_HOST, SERVER_PORT);

    resp_ctx_t ctx = { .buf = resp_buf, .buf_sz = resp_buf_sz, .len = 0, .overflow = false, .action_out = NULL };

    esp_http_client_config_t cfg = {
        .url            = url,
        .method         = HTTP_METHOD_POST,
        .event_handler  = http_evt,
        .user_data      = &ctx,
        .timeout_ms     = 60000, /* LLM ve TTS 30-40 sn sürebilir, 1 dk bekleme süresi */
        .buffer_size    = 4096,
        .buffer_size_tx = 4096,
        .keep_alive_enable = true,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) return ESP_FAIL;

    char payload[128];
    snprintf(payload, sizeof(payload), "{\"temperature\":%.1f, \"light\":%.1f, \"noise\":%.1f}", temp, light, noise);

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, payload, strlen(payload));

    ESP_LOGI(TAG, "Sensör verisi gönderiliyor: %s", payload);
    esp_err_t ret = esp_http_client_perform(client);

    if (ret == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        if (status == 200 && !ctx.overflow) {
            *resp_len = ctx.len;
            ESP_LOGI(TAG, "Proaktif uyarı yanıtı: %zu bayt WAV", ctx.len);
        } else if (status == 204) {
            *resp_len = 0;
            ESP_LOGI(TAG, "Sensör güncellendi, proaktif uyarı yok.");
        } else {
            ESP_LOGE(TAG, "Sunucu hatası: HTTP %d", status);
            ret = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "HTTP başarısız: %s", esp_err_to_name(ret));
    }

    esp_http_client_cleanup(client);
    return ret;
}
