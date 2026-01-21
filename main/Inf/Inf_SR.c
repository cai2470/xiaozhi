#include "Inf_SR.h"

esp_afe_sr_iface_t *afe_handle;
esp_afe_sr_data_t *afe_data;

void Inf_SR_Init(void)
{
    // 初始化模型
    srmodel_list_t *models = esp_srmodel_init("model");
    // 初始化AFE
    afe_config_t *afe_config = afe_config_init("M", models, AFE_TYPE_SR, AFE_MODE_LOW_COST);
    afe_config->aec_init = false;                                // 禁用回声消除
    afe_config->se_init = false;                                 // 禁用语音增强
    afe_config->ns_init = false;                                 // 禁用噪音抑制
    afe_config->vad_init = true;                                 // 启动语音活动检测
    afe_config->vad_mode = VAD_MODE_2;                           // 值越大,人声认定越严格
    afe_config->vad_min_speech_ms = 300;                         // 人声认定最短时间
    afe_config->vad_min_noise_ms = 300;                          // 静音认定最短时间
    afe_config->wakenet_init = true;                             // 启动唤醒词
    afe_config->wakenet_mode = DET_MODE_95;                      // 值越大,越不容易误触发
    afe_config->agc_init = false;                                // 关闭自动增益
    afe_config->memory_alloc_mode = AFE_MEMORY_ALLOC_MORE_PSRAM; // 使用外部sram

    // 获取句柄
    afe_handle = esp_afe_handle_from_config(afe_config);
    // 创建实例
    afe_data = afe_handle->create_from_config(afe_config);
}

// 获取需要喂给SR语音模型的一帧音频大小
int Inf_SR_GetChunkSize(void)
{
    return afe_handle->get_feed_chunksize(afe_data);
}

// 通过输入通道的个数
int Inf_SR_GetChannelNum(void)
{
    return afe_handle->get_feed_channel_num(afe_data);
}

// 将音频数据喂给SR语音模型
void Inf_SR_Feed(int16_t *datas)
{
    afe_handle->feed(afe_data, datas);
}

// 从SR语音模型获得结果
void Inf_SR_Fetch(afe_fetch_result_t **res)
{
    // afe_handle->fetch(afe_data) 返回的是一个指向结果结构体的指针
    // 我们通过 *res（解引用一次）找到外部那个真正的 res 指针变量
    // 然后把地址填进去
    if (res != NULL)
    {
        *res = afe_handle->fetch(afe_data);
    }
}