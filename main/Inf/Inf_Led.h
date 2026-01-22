#ifndef __INF_LED_H__
#define __INF_LED_H__
#include "led_strip.h"
#include <stdbool.h>

void Inf_Led_Init(void);
void Inf_Led_Open(void);
void Inf_Led_Close(void);

// 【新增】设置颜色接口
void Inf_Led_SetColor(int r, int g, int b);
bool Inf_Led_Is_Open(void);
void Inf_Led_GetColor(int *r, int *g, int *b);

#endif