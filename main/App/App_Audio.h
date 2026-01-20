#ifndef __APP_AUDIO_H__
#define __APP_AUDIO_H__
#include "Inf_ES8311.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Common_Debug.h"
#include "Common_Config.h"
#include "Inf_SR.h"
#include "esp_audio_dec.h"
#include "Inf_Encoder.h"
#include "Inf_Decoder.h"
#include "esp_heap_caps.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"


typedef void (*WakupCallback)(void);
typedef void (*VadStateChangeCallback)(VadChangeState );

void App_Audio_Init(WakupCallback wkcb,VadStateChangeCallback vadCb);

#endif
