#ifndef __INF_ENCODER_H__
#define __INF_ENCODER_H__
#include "esp_audio_enc_default.h"  // 默认编码器注册接口
#include "esp_audio_enc.h"          // 核心编码器 API
#include "esp_audio_enc_reg.h"      // 编码器注册相关
#include "esp_audio_types.h"
#include "esp_err.h"

void Inf_Encoder_Init(void);

void Inf_Encoder_GetSize(int *pcm_size, int *raw_size);

esp_err_t Inf_Encoder_Process(esp_audio_enc_in_frame_t *in_frame, esp_audio_enc_out_frame_t *out_frame);

#endif /* __INF_ENCODER_H__ */
