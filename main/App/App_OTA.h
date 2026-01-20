#ifndef __APP_OTA_H__
#define __APP_OTA_H__
#include "Driver_HttpClient.h"
#include "Common_Debug.h"
#include "Common_Config.h"
#include "nvs_flash.h"
#include "cJSON.h"

void App_OTA_Init(void);
/**
 * @brief 激活智能体
 * 
 */
void App_OTA_Activity(void);
#endif
