#ifndef __INF_LCD_H__
#define __INF_LCD_H__

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

#define LCD_H_RES              240
#define LCD_V_RES              320

extern esp_lcd_panel_io_handle_t io_handle;
extern esp_lcd_panel_handle_t panel_handle;
void Inf_Lcd_Init(void);
#endif

