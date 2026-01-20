#ifndef __INF_SR_H__
#define __INF_SR_H__

#include "esp_afe_sr_models.h"
#include "model_path.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"

void Inf_SR_Init(void);

int Inf_SR_GetChunkSize(void);

int Inf_SR_GetChannelNum(void);

void Inf_SR_Feed(int16_t *datas);

void Inf_SR_Fetch(afe_fetch_result_t **res);

#endif /* __INF_SR_H__ */