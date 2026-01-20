#include "Driver_HttpClient.h"

#include "esp_http_client.h"
static const char *TAG = "HTTP_CLIENT";

#define MAX_HTTP_OUTPUT_BUFFER 2048

esp_http_client_handle_t client;

ReceiveCallback receivecb;

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer = NULL; // Buffer to store response of http request from event handler
    static int output_len = 0;         // Stores number of bytes read
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
        // http请求出错同样释放内存
        if (output_buffer)
        {
            free(output_buffer);
            output_buffer = NULL;
        }
        output_len = 0;

        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        if (strcmp(evt->header_key, "Content-Length") == 0)
        {
            // 获取本次http响应的数据的长度
            uint16_t len = atoi(evt->header_value);
            // 申请内存存储响应数据
            output_buffer = (char *)heap_caps_malloc(len, MALLOC_CAP_SPIRAM);
        }

        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        // 将本次接收的数据存储到缓冲中
        memcpy(&output_buffer[output_len], (char *)evt->data, evt->data_len);
        output_len += evt->data_len;

        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
        if (receivecb)
        {
            receivecb(output_buffer, output_len);
        }
        //释放内存
        free(output_buffer);
        output_buffer = NULL;
        output_len = 0;

        break;

    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");

        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT");

        break;
    }
    return ESP_OK;
}

void Driver_HttpClient_Init(char *url, esp_http_client_method_t method)
{

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = _http_event_handler,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .method = method,
        .buffer_size = 128,
    };
    ESP_LOGI(TAG, "HTTP request with url =>");
    client = esp_http_client_init(&config);
}

// 发送一个请求
void Driver_HttpClient_Request(void)
{
    if (client)
    {
        esp_http_client_perform(client);
    }
}

// 添加一个请求头
void Driver_HttpClient_SetHeader(char *key, char *value)
{
    if (client)
    {
        esp_http_client_set_header(client, key, value);
    }
}

// 添加一个请求体
void Driver_HttpClient_SetBody(char *data, int len)
{
    if (client)
    {
        esp_http_client_set_post_field(client, data, len);
    }
}

// 注册一个接收到http响应的一个回调函数
void Driver_HttpClient_RegisterHandleDataCallback(ReceiveCallback cb)
{
    receivecb = cb;
}