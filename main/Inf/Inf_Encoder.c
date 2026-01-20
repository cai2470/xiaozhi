#include "Inf_Encoder.h"

static esp_audio_enc_handle_t encoder_handle = NULL;

void Inf_Encoder_Init(void)
{
    // 1. 注册编码器 (参考 audio_encoder_test.c 第135行)
    esp_audio_enc_register_default();

    // 1. 先用默认宏“打底”
    // 这行代码会把所有参数都填上默认值
    esp_opus_enc_config_t opus_cfg = ESP_OPUS_ENC_CONFIG_DEFAULT();

    // 2. 【关键】根据你的硬件(ES8311)和业务手动“覆盖”修改
    // 必须去确认你的 Inf_ES8311_Init 里配的是多少！通常是 16000
    opus_cfg.sample_rate = 16000;

    // 必须确认你的麦克风数量！通常是 1
    opus_cfg.channel = 1;

    // 码率：决定音质和流量。
    // 语音对讲一般 24000 (24kbps) ~ 32000 (32kbps) 就很清晰了
    // 默认宏可能是 64000 或 96000，对于语音来说太浪费流量了
    opus_cfg.bitrate = 32000;

    // 配置一帧声音数据的长度
    opus_cfg.frame_duration = ESP_OPUS_ENC_FRAME_DURATION_60_MS;

    // 3. 这里的应用模式也可以改
    // VOIP: 侧重低延迟，适合实时通话 (推荐)
    // AUDIO: 侧重高保真，适合听歌
    opus_cfg.application_mode = ESP_OPUS_ENC_APPLICATION_VOIP;

    // 4. 打包进通用信封
    esp_audio_enc_config_t enc_cfg = {
        .type = ESP_AUDIO_TYPE_OPUS,
        .cfg = &opus_cfg,
        .cfg_sz = sizeof(opus_cfg),
    };

    // 5. 打开
    esp_audio_enc_open(&enc_cfg, &encoder_handle);
};

//(获取缓冲区大小)
void Inf_Encoder_GetSize(int *pcm_size, int *raw_size)
{
    if (encoder_handle)
    {
        // 直接调用官方 API 获取需要的输入和输出 buffer 大小
        esp_audio_enc_get_frame_size(encoder_handle, pcm_size, raw_size);
    }
}

esp_err_t Inf_Encoder_Process(esp_audio_enc_in_frame_t *in_frame, esp_audio_enc_out_frame_t *out_frame)
{
    if (encoder_handle == NULL) {
        return ESP_FAIL;
    }
    
    // 直接调用官方处理函数
    return esp_audio_enc_process(encoder_handle, in_frame, out_frame);
}