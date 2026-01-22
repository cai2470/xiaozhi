#ifndef __APP_COMMUNICATION_H__
#define __APP_COMMUNICATION_H__
#include "Common_Config.h"
#include "Driver_Websocket.h"
#include "Common_Debug.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "App_Display.h"
#include "Inf_IOT.h"
#include "Inf_Led.h"

void App_Communication_Init(void);
bool App_Communication_IsConnected(void);
void App_Communication_ConnectServer(void);
void App_Communication_SendHello(void);
void App_Communication_SendWakup(void);
void App_Communication_StartListing(void);
void App_Communication_StopListing(void);
void App_Communication_Abort(void);
void App_Communication_PushStatus(void);
#endif
