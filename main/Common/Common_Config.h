#ifndef __COMMON_CONFIG_H__
#define __COMMON_CONFIG_H__
/* 1. 必须先引用 FreeRTOS.h */
#include "freertos/FreeRTOS.h"
/* 2. 然后才是其他 FreeRTOS 组件 */
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "Common_Debug.h"
#include "freertos/ringbuf.h"
#include <stdbool.h>




//wifi连接成功标志
#define WIFI_CONNECTED (1 << 0)
//http请求错误标志
#define HTTP_REQUEST_ERROR (1 << 1)
//激活成功
#define ACTIVATION_SUCCESS (1 << 2)
//没有激活
#define ACTIVATION_FAIL (1 << 3)
//激活响应数据错误
#define ACTIVATION_DATA_ERROR (1 << 4)
//websocket连接成功
#define WEBSOCKET_CONNECT_SUCCESS (1 << 5)
//收到hello响应
#define WEBSOCKET_HELLO_RESPONSE (1 << 6)

extern RingbufHandle_t es8311_to_sr_buffer, sr_to_encoder_buff, encoder_to_ws_buff, ws_to_decoder_buff;

typedef enum{
    SILENCE_TO_SPEECH, //从静音变说话
    SPEECH_TO_SILENCE, //从说话变静音
} VadChangeState;

typedef enum {
    WEBSOCKET_TEXT_DATA,
    WEBSOCKET_BIN_DATA
} WebsocketDataType;


typedef enum{
    IDLE, //空闲
    SPEAKING, //小智说话中
    LISTING, //用户说话中
}CommunicationStatus;

extern CommunicationStatus communicationStatus;

// 设备ID
extern uint8_t device_id[18];
// 客户端id
extern uint8_t client_id[37];

extern EventGroupHandle_t global_event;

//websocket url
extern char* websocket_url;
extern char* token;
extern char* activation_code;

extern bool is_wakup;
extern char* session_id;




#define SERVER_HTTP_URL "https://api.tenclass.net/xiaozhi/ota/"

#endif /* __COMMON_CONFIG_H__ */