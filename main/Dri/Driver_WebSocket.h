#ifndef __DRIVER_WEBSOCKET_H__
#define __DRIVER_WEBSOCKET_H__
#include "esp_websocket_client.h"
#include "esp_event.h"
#include "esp_crt_bundle.h"
#include "Common_Config.h"

typedef void (* WebsocketReceiveHandleFunc)( char*,int ,WebsocketDataType);
typedef void (* WebsocketFinishHandleFunc)(void);

void Driver_Websocket_Init(char *url);

void Driver_Websocket_Start(void);

void Driver_Websocket_Stop(void);

bool Driver_Websocket_IsConnected(void);

void Driver_Websocket_Send(char *datas, int len, WebsocketDataType type);

void Driver_Websocket_AppendHeader(char *key, char *value);

void Driver_Websocket_RegisterCallback(WebsocketReceiveHandleFunc rcb, WebsocketFinishHandleFunc fcb);
#endif
