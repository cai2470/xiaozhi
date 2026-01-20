#ifndef __DRIVER_HTTPCLIENT_H__
#define __DRIVER_HTTPCLIENT_H__

#include <string.h>
#include "esp_crt_bundle.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_http_client.h"
typedef void (*ReceiveCallback)(char *, int);

void Driver_HttpClient_Init(char *url, esp_http_client_method_t method);
void Driver_HttpClient_Request(void);
void Driver_HttpClient_SetHeader(char *key, char *value);

void Driver_HttpClient_SetBody(char *data, int len);

void Driver_HttpClient_RegisterHandleDataCallback(ReceiveCallback cb);

#endif /* __DRIVER_HTTPCLIENT_H__ */