
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

extern bool is_wakup;

static void App_Application_KeyCallback(void *button_handle, void *user_data);

static void App_Application_WIFIConnectedCallback(void);

static void App_Application_QrCodeCallback(const char *payload);

static void App_Application_CreateRingBuffer(void);

static void App_Application_Wakup(void);

static void App_Application_VadChange(VadChangeState state);

void App_Application_Start(void)
{

    // 创建一个全局的事件标志组以此来等待wifi连接成功
    global_event = xEventGroupCreate();

    // 初始化按钮
    Inf_key_Init();

    // 注册按钮的回调函数
    Inf_key_RegisterKey1Callbacks(BUTTON_SINGLE_CLICK, App_Application_KeyCallback, (void *)KEY1);
    Inf_key_RegisterKey2Callbacks(BUTTON_LONG_PRESS_UP, App_Application_KeyCallback, (void *)KEY2);
    // 注册wifi的回调函数
    Driver_WIFI_RegisterCallback(App_Application_WIFIConnectedCallback);
    // 注册二维码的回调函数
    Driver_WIFI_RegisterShowQrCodeCallback(App_Application_QrCodeCallback);

    // 初始化WIFI
    Driver_WIFI_Init();

    // 等待WiFi连接成功的等待函数，就是进入阻塞态的函数
    xEventGroupWaitBits(global_event, WIFI_CONNECTED, pdFALSE, pdTRUE, portMAX_DELAY);

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




    
    // // 创建Http服务请求的初始化
    // Driver_HttpClient_Init("https://api.tenclass.net/xiaozhi/ota/", HTTP_METHOD_POST);
    // // 发送请求头
    // Driver_HttpClient_SetHeader("Content-Type", "application/json");
    // Driver_HttpClient_SetHeader("User-Agent", "bread-compact-wifi-128x64/1.0.1");
    // Driver_HttpClient_SetHeader("Device-Id", "11:99:33:19:55:77");
    // Driver_HttpClient_SetHeader("Client-Id", "7b94d69a-9808-4c59-9c9b-704333b38aff");

    // 【修正点1】填入压缩并转义后的 JSON 数据
    // C语言字符串里双引号 " 需要写成 \"
    // char *body = "{\"application\":{\"version\":\"1.0.1\",\"elf_sha256\":\"c8a8ecb6d6fbcda682494d9675cd1ead240ecf38bdde75282a42365a0e396033\"},\"board\":{\"type\":\"bread-compact-wifi\",\"name\":\"bread-compact-wifi-128x64\",\"ssid\":\"卧室\",\"rssi\":-55,\"channel\":1,\"ip\":\"192.168.1.11\",\"mac\":\"11:22:33:44:55:66\"}}";
    // Driver_HttpClient_SetBody(body, strlen(body));
    // Driver_HttpClient_Request();
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

    // 设置事件标志，表示WIFI连接成功
    xEventGroupSetBits(global_event, WIFI_CONNECTED);
}

static void App_Application_QrCodeCallback(const char *payload)
{
    MyLogE("QR Code Payload: %s", payload);
}

/**
 * @brief 清除缓冲区数据，防止旧会话残留数据干扰新交互
 */
static void App_Application_ClearRingBuffer(RingbufHandle_t buff)
{
    size_t size = 0;
    void* data = NULL;
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
        //服务器放弃说话
        App_Communication_Abort();
        break;
    default:
        break;
    }

    //清除缓冲区中上一个唤醒词之后没有发完的数据
    App_Application_ClearRingBuffer(sr_to_encoder_buff);
    App_Application_ClearRingBuffer(encoder_to_ws_buff);

    communicationStatus = IDLE;
    App_Communication_SendWakup();
}
// Vad状态变化
// Vad状态变化
static void App_Application_VadChange(VadChangeState state)
{
    //MyLogE("%s", state == SPEECH_TO_SILENCE ? "说话变静音" : "静音变说话");

    if( is_wakup && state == SILENCE_TO_SPEECH && communicationStatus!= SPEAKING ){
        MyLogE("开始监听...");
        //用户在说话, 小智没说话
        communicationStatus = LISTING;
        //开始监听
        App_Communication_StartListing();
    }else if( is_wakup && state == SPEECH_TO_SILENCE && communicationStatus == LISTING ){
        communicationStatus = IDLE;
        MyLogE("结束监听...");
        //结束监听
        App_Communication_StopListing();
    }
}
