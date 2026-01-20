#ifndef __INF_DECODER_H__
#define __INF_DECODER_H__
#include "esp_audio_dec_default.h"
#include "esp_audio_dec.h"

void Inf_Decoder_Init(void);

esp_audio_err_t Inf_Decoder_Process(esp_audio_dec_in_raw_t *in_raw, esp_audio_dec_out_frame_t *out_frame);
#endif

