#ifndef __INF_ES8311_H__
#define __INF_ES8311_H__
#include "driver/i2c_master.h"
#include "driver/i2s_std.h"
#include "driver/i2s_tdm.h"
#include "soc/soc_caps.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"

void Inf_ES8311_Init(void);
void Inf_ES8311_SetVolume(int volume);
int Inf_ES8311_GetVolume(void);
void Inf_ES8311_Open(void);
void Inf_ES8311_Close(void);
int Inf_ES8311_Read(uint8_t *datas, int len);
int Inf_ES8311_Write(uint8_t *datas, int len);



#endif /* __INF_ES8311_H__ */