#include "Inf_Decoder.h"

esp_audio_dec_handle_t decoder;

void Inf_Decoder_Init(void){
    // Configuration for AAC decoder
    esp_opus_dec_cfg_t conf = {
        .sample_rate = 16000, //音频采样率
        .channel = 1,
        .frame_duration = ESP_OPUS_DEC_FRAME_DURATION_60_MS,  
    };

    //打开解码器
    esp_opus_dec_open(&conf, sizeof(conf), &decoder);
}

/**
 * @brief 解码
 * 
 * @param in_raw 
 * @param out_frame 
 * @return esp_audio_err_t 
 */
esp_audio_err_t Inf_Decoder_Process(esp_audio_dec_in_raw_t* in_raw,esp_audio_dec_out_frame_t* out_frame){
    esp_audio_dec_info_t info;
    return esp_opus_dec_decode(decoder,in_raw,out_frame,&info);
}