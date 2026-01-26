
#include <stdio.h>
#include <string.h>
/* --- 1. 系统/框架库放最上面 --- */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
/* --- 2. 引入各个模块的头文件 (解决 KEY1, WIFI 等未定义报错) --- */
#include "Inf_key.h"       // 定义了 KeyNum, KEY1, KEY2
#include "Driver_WIFI.h"   // 定义了 Driver_WIFI_Init
#include "Common_Config.h" // 定义了 global_event
#include "Common_Debug.h"  // 定义了 MyLogE
/* --- 3. 最后引入本模块头文件 --- */
#include "App_Application.h"
#include "Driver_HttpClient.h"
#include "App_OTA.h"
#include "App_Audio.h"
#include "App_Communication.h"
#include "App_Display.h"
#include "Inf_Led.h"

extern bool is_wakup;

static void App_Application_KeyCallback(void *button_handle, void *user_data);

static void App_Application_WIFIConnectedCallback(void);

static void App_Application_WifiMonitorTask(void *pvParameters);

static void App_Application_CreateRingBuffer(void);

static void App_Application_Wakup(void);

static void App_Application_VadChange(VadChangeState state);

static void App_Application_ShowQrCode(const char *payload);

void App_Application_Start(void)
{

    // 创建一个全局的事件标志组以此来等待wifi连接成功
    global_event = xEventGroupCreate();

    // 初始化按钮
    Inf_key_Init();
    Inf_Led_Init();

    // 初始化显示任务
    App_Display_Init();
    App_Display_SetTitleText("配网中...");

    // 注册显示二维码的回调函数
    Driver_WIFI_RegisterShowQrCodeCallback(App_Application_ShowQrCode);

    // 注册按钮的回调函数
    Inf_key_RegisterKey1Callbacks(BUTTON_SINGLE_CLICK, App_Application_KeyCallback, (void *)KEY1);
    Inf_key_RegisterKey2Callbacks(BUTTON_LONG_PRESS_UP, App_Application_KeyCallback, (void *)KEY2);
    // 注册wifi的回调函数
    Driver_WIFI_RegisterCallback(App_Application_WIFIConnectedCallback);

    // 初始化WIFI
    MyLogI("正在初始化 WiFi...");
    Driver_WIFI_Init();

    // 等待WiFi连接成功的等待函数，就是进入阻塞态的函数
    xEventGroupWaitBits(global_event, WIFI_CONNECTED, pdFALSE, pdTRUE, portMAX_DELAY);
    MyLogI("WiFi 已就绪，继续启动流程");
    App_Display_SetTitleText("启动中...");
    // 删除二维码
    App_Display_DeleteQRCode();

    // OTA初始化
    App_OTA_Init();
    // 激活智能体
    App_OTA_Activity();

    // 创建需要的环形缓存区
    App_Application_CreateRingBuffer();

    // 初始化音频处理
    App_Audio_Init(App_Application_Wakup, App_Application_VadChange);

    // 初始化通信模块 (WebSocket)
    App_Communication_Init();

    App_Display_SetContentText("请使用\"你好,小智\"唤醒");

    // 启动 WiFi 信号监控任务，每 5 秒更新一次
    xTaskCreate(App_Application_WifiMonitorTask, "wifi_monitor", 2048, NULL, 3, NULL);
}

// 按键的回调函数
static void App_Application_KeyCallback(void *button_handle, void *user_data)
{
    KeyNum key = (KeyNum)user_data;

    if (key == KEY1)
    {
        MyLogE("KEY1...");
    }
    else
    {
        // 擦除之前配网重启
        Driver_WIFI_Reset_Provisioning();
        esp_restart();
    }
}

static void App_Application_WIFIConnectedCallback(void)
{
    MyLogE("WIFI 连接成功!");

    App_Display_SetWifiIcon(Driver_WIFI_GetRSSI());

    // 设置事件标志，表示WIFI连接成功
    xEventGroupSetBits(global_event, WIFI_CONNECTED);
}

/**
 * @brief WiFi 信号强度监控任务
 */
static void App_Application_WifiMonitorTask(void *pvParameters)
{
    while (1) {
        // 使用 WaitBits 代替 GetBits + Delay，可以更优雅地处理连接断开的情况
        EventBits_t bits = xEventGroupWaitBits(global_event, WIFI_CONNECTED, pdFALSE, pdTRUE, pdMS_TO_TICKS(5000));
        if (bits & WIFI_CONNECTED) {
            App_Display_SetWifiIcon(Driver_WIFI_GetRSSI());
        }
        // 如果连接断开，WaitBits 会在 5s 后超时，循环继续，直到再次连接
    }
}

/**
 * @brief 清除缓冲区数据，防止旧会话残留数据干扰新交互
 */
static void App_Application_ClearRingBuffer(RingbufHandle_t buff)
{
    size_t size = 0;
    void *data = NULL;
    while (1)
    {
        data = xRingbufferReceive(buff, &size, 0);
        if (data == NULL)
        {
            break;
        }
        vRingbufferReturnItem(buff, data);
    }
}

static void App_Application_CreateRingBuffer(void)
{
    es8311_to_sr_buffer = xRingbufferCreateWithCaps(16 * 1024, RINGBUF_TYPE_BYTEBUF, MALLOC_CAP_SPIRAM);
    sr_to_encoder_buff = xRingbufferCreateWithCaps(16 * 1024, RINGBUF_TYPE_BYTEBUF, MALLOC_CAP_SPIRAM);
    // 编码后的 Opus 帧和接收到的音频包建议使用 NOSPLIT，确保单次读取数据的完整性
    encoder_to_ws_buff = xRingbufferCreateWithCaps(16 * 1024, RINGBUF_TYPE_NOSPLIT, MALLOC_CAP_SPIRAM);
    ws_to_decoder_buff = xRingbufferCreateWithCaps(16 * 1024, RINGBUF_TYPE_NOSPLIT, MALLOC_CAP_SPIRAM);
}

// 唤醒
static void App_Application_Wakup(void)
{
    is_wakup = true; // 确保唤醒标志位在逻辑开始时明确置位

    switch (communicationStatus)
    {
    case IDLE:
        if (!App_Communication_IsConnected())
        {
            // 向服务器发起连接
            App_Communication_ConnectServer();
            // 发送hello
            App_Communication_SendHello();
        }
        break;
    case SPEAKING:
        // 服务器放弃说话
        App_Communication_Abort();
        break;
    default:
        break;
    }

    // 清除缓冲区中上一个唤醒词之后没有发完的数据
    App_Application_ClearRingBuffer(sr_to_encoder_buff);
    App_Application_ClearRingBuffer(encoder_to_ws_buff);

    // 状态重置为空闲，准备接收 VAD 切换到 LISTING
    communicationStatus = IDLE;
    App_Communication_SendWakup();
}
// Vad状态变化
// Vad状态变化
static void App_Application_VadChange(VadChangeState state)
{
    // MyLogE("%s", state == SPEECH_TO_SILENCE ? "说话变静音" : "静音变说话");

    if (is_wakup && state == SILENCE_TO_SPEECH && communicationStatus != SPEAKING)
    {
        MyLogE("开始监听...");
        // 用户在说话, 小智没说话
        communicationStatus = LISTING;
        // 开始监听
        App_Communication_StartListing();
        App_Display_SetTitleText("小智正在听...");
    }
    else if (is_wakup && state == SPEECH_TO_SILENCE && communicationStatus == LISTING)
    {
        communicationStatus = IDLE;
        MyLogE("结束监听...");
        // 结束监听
        App_Communication_StopListing();
    }
}
static void App_Application_ShowQrCode(const char *payload)
{
    App_Display_ShowQRCode((void *)payload, strlen(payload));
}
