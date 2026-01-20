#include <string.h>
#include "App_Audio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "esp_heap_caps.h"

#define ES8311_2_Buffer_Task_Name "es2buffer"
#define ES8311_2_Buffer_Task_StackDepth 4096
#define ES8311_2_Buffer_Task_Priority 5
TaskHandle_t es2bufferHandle;

#define Buffer_2_SR_Task_Name "buffer2sr"
#define Buffer_2_SR_Task_StackDepth 8192
#define Buffer_2_SR_Task_Priority 5
TaskHandle_t buffer2srHandle;

#define SR_2_Buffer_Task_Name "sr2buffer"
#define SR_2_Buffer_Task_StackDepth 8192
#define SR_2_Buffer_Task_Priority 5
TaskHandle_t sr2bufferHandle;

#define Buffer_2_Encoder_Task_Name "buffer2encoder"
#define Buffer_2_Encoder_Task_StackDepth 40*1024
#define Buffer_2_Encoder_Task_Priority 5
TaskHandle_t buffer2encoderHandle;

#define Buffer_2_Decoder_Task_Name "buffer2decoder"
#define Buffer_2_Decoder_Task_StackDepth 40*1024
#define Buffer_2_Decoder_Task_Priority 5
TaskHandle_t buffer2decoderHandle;

static WakupCallback g_wkcb = NULL;
static VadStateChangeCallback g_vadCb = NULL;


static void App_Audio_ReadEs8311ToBufferTaskFunc(void *args);
static void App_Audio_ReadBufferToSRTaskFunc(void *args);
static void App_Audio_SRToBufferTaskFunc(void *args);
static void App_Audio_BufferToEncoderTaskFunc(void *args);
static void App_Audio_BufferToDecoderTaskFunc(void *args);

/**
 * @brief 从环形缓冲区中读取指定长度的数据，处理回绕并拼凑到连续内存中
 */
static void App_Audio_CopyRingBuffer(RingbufHandle_t handle, uint8_t *buff, int size)
{
    size_t len = 0;
    int index = 0;
    while (size > 0)
    {
        uint8_t *data = (uint8_t *)xRingbufferReceiveUpTo(handle, &len, portMAX_DELAY, (size_t)size);
        if (data != NULL) {
            memcpy(&buff[index], data, len);
            size -= len;
            index += len;
            vRingbufferReturnItem(handle, data);
        }
    }
}

void App_Audio_Init(WakupCallback wkcb, VadStateChangeCallback vadCb)
{
    g_wkcb = wkcb;
    g_vadCb = vadCb;

    // 初始化ES8311
    Inf_ES8311_Init();
    Inf_ES8311_Open();

    // 初始化SR
    Inf_SR_Init();

    // 初始化编解码
    Inf_Encoder_Init();
    Inf_Decoder_Init();

    // 创建一个任务将，处理ES8311到SR识别的过程放入缓冲区的任务
    xTaskCreateWithCaps(App_Audio_ReadEs8311ToBufferTaskFunc, ES8311_2_Buffer_Task_Name, ES8311_2_Buffer_Task_StackDepth, NULL, ES8311_2_Buffer_Task_Priority, &es2bufferHandle, MALLOC_CAP_SPIRAM);
    // 将从缓存区的任务拿出来交给SR识别任务
    xTaskCreateWithCaps(App_Audio_ReadBufferToSRTaskFunc, Buffer_2_SR_Task_Name, Buffer_2_SR_Task_StackDepth, NULL, Buffer_2_SR_Task_Priority, &buffer2srHandle, MALLOC_CAP_SPIRAM);
    // 创建取出SR识别结果放入缓冲任务
    xTaskCreateWithCaps(App_Audio_SRToBufferTaskFunc, SR_2_Buffer_Task_Name, SR_2_Buffer_Task_StackDepth, NULL, SR_2_Buffer_Task_Priority, &sr2bufferHandle, MALLOC_CAP_SPIRAM);
    // 将缓冲数据取出来进行编码任务
    xTaskCreateWithCaps(App_Audio_BufferToEncoderTaskFunc, Buffer_2_Encoder_Task_Name, Buffer_2_Encoder_Task_StackDepth, NULL, Buffer_2_Encoder_Task_Priority, &buffer2encoderHandle, MALLOC_CAP_SPIRAM);
    // 将缓冲数据取出解码播放任务
    xTaskCreateWithCaps(App_Audio_BufferToDecoderTaskFunc, Buffer_2_Decoder_Task_Name, Buffer_2_Decoder_Task_StackDepth, NULL, Buffer_2_Decoder_Task_Priority, &buffer2decoderHandle, MALLOC_CAP_SPIRAM);
}

static void App_Audio_ReadEs8311ToBufferTaskFunc(void *args)
{
    MyLogE("启动ES8311开始接收数据放入缓冲区内");

    uint8_t datas[512] = {0};
    while (1)
    {
        // 读取ES8311的麦克风数据
        if (Inf_ES8311_Read(datas, 512) == ESP_CODEC_DEV_OK)
        {
            // 修正：增加第一个参数 handle
            xRingbufferSend(es8311_to_sr_buffer, datas, 512, portMAX_DELAY);
        }
        vTaskDelay(1);
    }
}

static void App_Audio_ReadBufferToSRTaskFunc(void *args)
{
    MyLogE("启动将缓冲数据取出来交给SR识别任务");

    int ChunkSize = Inf_SR_GetChunkSize();
    int size_bytes = ChunkSize * sizeof(int16_t); // SR 引擎单次需要的字节数

    // 申请本地缓冲区
    int16_t *sr_input_buf = (int16_t *)heap_caps_malloc(size_bytes, MALLOC_CAP_SPIRAM);

    while (1)
    {
        // 使用封装好的函数，源源不断地凑齐一帧 SR 数据
        App_Audio_CopyRingBuffer(es8311_to_sr_buffer, (uint8_t *)sr_input_buf, size_bytes);

        // 喂给 SR 引擎处理
        Inf_SR_Feed(sr_input_buf);
    }
}

static void App_Audio_SRToBufferTaskFunc(void *args)
{
    MyLogE("启动取出SR识别结果放入缓冲任务");

    afe_fetch_result_t *res = NULL;
    // 上一次的vad状态
    int lastState = VAD_SILENCE;
    while (1)
    {
        // 取出SR的结果 (阻塞调用，直到算法处理完一帧)
        Inf_SR_Fetch(&res);

        if (res == NULL) continue;

        // 1. 判断是否唤醒
        if (res->wakeup_state == WAKENET_DETECTED)
        {
            is_wakup = true;
            // 注意: 在每次说唤醒词的时候 将状态切成静音,否则可能出现语音被吞的情况
            lastState = VAD_SILENCE;
            // 执行唤醒词回调
            if (g_wkcb)
            {
                g_wkcb();
            }
        }

        // 2. 唤醒并且vad状态变化
        if (is_wakup && lastState != res->vad_state)
        {
            lastState = res->vad_state;
            if (g_vadCb)
            {
                g_vadCb(lastState == VAD_SILENCE ? SPEECH_TO_SILENCE : SILENCE_TO_SPEECH);
            }
        }

        // 3. 只有唤醒了 并且是说话才将语音放入缓冲
        if (is_wakup && res->vad_state == VAD_SPEECH)
        {
            // 先将SR缓冲数据放入,否则会出现数据丢失 (vad_cache 包含了检测到人声前的一小段音频)
            if (res->vad_cache_size > 0)
            {
                if (xRingbufferSend(sr_to_encoder_buff, res->vad_cache, res->vad_cache_size, 0) == pdFALSE)
                {
                    MyLogW("sr_to_encoder_buff 满了，丢弃了 vad_cache 数据");
                }
            }
            // 将语音识别后的音频放入缓冲
            if (xRingbufferSend(sr_to_encoder_buff, res->data, res->data_size, 0) == pdFALSE)
            {
                MyLogW("sr_to_encoder_buff 满了，丢弃了音频数据");
            }
        }
    }
}

static void App_Audio_BufferToDecoderTaskFunc(void *args)
{
    MyLogE("启动将缓冲数据取出解码播放任务");

    // 需要解码的数据结构
    esp_audio_dec_in_raw_t raw;
    // 用于存储解码后的pcm数据，初始分配 10KB
    esp_audio_dec_out_frame_t out = {
        .buffer = heap_caps_malloc(10 * 1024, MALLOC_CAP_SPIRAM),
        .len = 10 * 1024};
    esp_audio_err_t error;
    uint8_t* datas = NULL;

    while (1)
    {
        // 1. 从缓冲获取数据 (阻塞等待 WebSocket 收到音频包)
        datas = (uint8_t *)xRingbufferReceive(ws_to_decoder_buff, (size_t*)&raw.len, portMAX_DELAY);
        raw.buffer = datas;

        // 2. 循环解码：一包数据可能包含多个音频帧
        while (raw.len > 0)
        {
            error = Inf_Decoder_Process(&raw, &out);
            if (error == ESP_AUDIO_ERR_BUFF_NOT_ENOUGH)
            {
                // 缓冲区不够，动态扩容
                out.buffer = (uint8_t *)heap_caps_realloc(out.buffer, out.needed_size, MALLOC_CAP_SPIRAM);
                out.len = out.needed_size;
                // 扩容后不需要 break，继续尝试解码当前帧
            }
            else if (error == ESP_AUDIO_ERR_OK)
            {
                // 3. 播放解码后的 PCM 音频
                Inf_ES8311_Write(out.buffer, out.decoded_size);
                // 更新指针，准备处理下一帧
                raw.len -= raw.consumed;
                raw.buffer += raw.consumed;
            }
            else {
                // 遇到无法解析的错误，跳出循环处理下一包
                break;
            }
        }

        // 4. 处理完一包后，必须回收 RingBuffer 内存
        vRingbufferReturnItem(ws_to_decoder_buff, datas);
    }
}

static void App_Audio_BufferToEncoderTaskFunc(void *args)
{
    MyLogE("启动将缓冲数据取出进行编码任务");

    // 获取编码器需要的 PCM 输入大小和编码后的 Raw 输出大小
    int pcm_size = 0;
    int raw_size = 0;
    Inf_Encoder_GetSize(&pcm_size, &raw_size);

    // 创建两块内存,一块用于存储pcm数据,一块用于存储编码后的数据
    uint8_t *pcm_data = heap_caps_malloc(pcm_size, MALLOC_CAP_SPIRAM);
    uint8_t *raw_data = heap_caps_malloc(raw_size, MALLOC_CAP_SPIRAM);

    esp_audio_enc_in_frame_t in_frame = {
        .buffer = pcm_data,
        .len = pcm_size,
    };
    esp_audio_enc_out_frame_t out_frame = {
        .buffer = raw_data,
        .len = raw_size,
    };

    while (1)
    {
        // 1. 从环形缓冲区取出 PCM 数据。如果缓冲区数据不够，这里会阻塞等待。
        App_Audio_CopyRingBuffer(sr_to_encoder_buff, pcm_data, pcm_size);

        // 2. 执行编码 (PCM -> Opus)
        if (Inf_Encoder_Process(&in_frame, &out_frame) == ESP_AUDIO_ERR_OK)
        {
            // 3. 将编码后的数据存入下一个缓冲区，准备交给 WebSocket 发送
            if (xRingbufferSend(encoder_to_ws_buff, raw_data, out_frame.encoded_bytes, 0) == pdFALSE)
            {
                MyLogW("encoder_to_ws_buff 满了，丢弃了编码后的数据");
            }
        }
    }
}