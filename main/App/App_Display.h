#ifndef __APP_DISPLAY_H__
#define __APP_DISPLAY_H__
#include "Inf_Lcd.h"
#include "esp_lvgl_port.h"

void App_Display_Init(void);
void App_Display_SetTitleText(char *datas);
void App_Display_SetContentText(char *datas);
void App_Display_SetEmojiText(char *datas);
void App_Display_ShowQRCode(void *datas, size_t len);
void App_Display_DeleteQRCode(void);
#endif
