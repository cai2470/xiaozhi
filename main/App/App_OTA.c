#include "App_OTA.h"
#include "Driver_HttpClient.h"
#include "esp_mac.h"
#include <stdio.h>
#include "esp_random.h" // 包含硬件随机数生成接口
#include <stdint.h>
#include "esp_app_desc.h"
#include "Common_Config.h"

static void App_OTA_HttpReceiveHandle(char *datas, int len);
static void App_OTA_SetHeader(void);
static void App_OTA_GetClientId(void);
static void App_OTA_SetBodyAndRequest(void);
void generate_uuid_v4(char *out_str);

void App_OTA_Init(void)
{
    // 初始化httpclient
    Driver_HttpClient_Init(SERVER_HTTP_URL, HTTP_METHOD_POST);

    // 注册收到http响应的处理回调
    Driver_HttpClient_RegisterHandleDataCallback(App_OTA_HttpReceiveHandle);
}
void App_OTA_Activity(void)
{

    App_OTA_SetHeader();

    App_OTA_SetBodyAndRequest();
}

static void App_OTA_HttpReceiveHandle(char *datas, int len)
{
    MyLogE("收到数据=>> %.*s", len, datas);

    cJSON *root = cJSON_ParseWithLength(datas, (size_t)len);
    if (root == NULL)
    {
        MyLogE("JSON 格式错误");
        xEventGroupSetBits(global_event, ACTIVATION_DATA_ERROR);
        return;
    }

    cJSON *websocket = cJSON_GetObjectItemCaseSensitive(root, "websocket");
    if (websocket == NULL)
    {
        MyLogE("未找到 websocket 字段");
        xEventGroupSetBits(global_event, ACTIVATION_DATA_ERROR);
        cJSON_Delete(root);
        return;
    }

    cJSON *url_obj = cJSON_GetObjectItemCaseSensitive(websocket, "url");
    cJSON *token_obj = cJSON_GetObjectItemCaseSensitive(websocket, "token");

    if (cJSON_IsString(url_obj) && cJSON_IsString(token_obj))
    {
        if (websocket_url) free(websocket_url);
        websocket_url = strdup(url_obj->valuestring);
        
        if (token) free(token);
        token = strdup(token_obj->valuestring);
    }

    // 处理激活逻辑
    cJSON *activation = cJSON_GetObjectItemCaseSensitive(root, "activation");
    if (activation == NULL)
    {
        // 没有 activation 字段，说明已经激活成功
        MyLogI("设备已激活");
        xEventGroupSetBits(global_event, ACTIVATION_SUCCESS);
    }
    else
    {
        // 存在 activation 字段，说明需要激活
        cJSON *code = cJSON_GetObjectItemCaseSensitive(activation, "code");
        if (cJSON_IsString(code))
        {
            if (activation_code) free(activation_code);
            activation_code = strdup(code->valuestring);
            MyLogW("设备未激活，激活码: %s", activation_code);
        }
        xEventGroupSetBits(global_event, ACTIVATION_FAIL);
    }

    cJSON_Delete(root);
}

static void App_OTA_SetHeader(void)
{

    // 获取mac地址
    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    sprintf((char *)device_id, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    // 获取client_id
    App_OTA_GetClientId();

    // 设置发送的数据类型
    Driver_HttpClient_SetHeader("Content-Type", "application/json");
    // 板子身份信息
    Driver_HttpClient_SetHeader("User-Agent", "bread-compact-wifi-128x64/1.0.1");
    // 设备ID
    Driver_HttpClient_SetHeader("Device-Id", (char *)device_id);
    // 客户端id
    Driver_HttpClient_SetHeader("Client-Id", (char *)client_id);

    MyLogE("divice_id=%s client_id=%s", device_id, client_id);
}

static void App_OTA_GetClientId(void)
{

    // 1、打开NVS命名空间
    nvs_handle_t handle;
    nvs_open("uuid", NVS_READWRITE, &handle);

    // 2、查询clientid在命名空间中是否存在
    size_t len = 0;
    nvs_get_str(handle, "clientid", NULL, &len);

    if (len > 0)
    {
        MyLogE("存在client_id");
        // nvs中存在clientid,取出
        nvs_get_str(handle, "clientid", (char *)client_id, &len);
    }
    else
    {
        // nvs中没有clientid
        MyLogE("不存在client_id");
        // 3、生成clientid
        generate_uuid_v4((char *)client_id);

        // 4、保存clientid到nvs中
        nvs_set_str(handle, "clientid", (char *)client_id);

        // 5、提交
        nvs_commit(handle);
    }

    // 关闭nvs
    nvs_close(handle);
}

static void App_OTA_SetBodyAndRequest(void)
{

    cJSON *root = cJSON_CreateObject();
    cJSON *application = cJSON_CreateObject();
    cJSON_AddStringToObject(application, "version", "1.0.1");
    cJSON_AddStringToObject(application, "elf_sha256", esp_app_get_elf_sha256_str());

    cJSON_AddItemToObject(root, "application", application);

    cJSON *board = cJSON_CreateObject();
    cJSON_AddStringToObject(board, "type", "bread-compact-wifi");
    cJSON_AddStringToObject(board, "name", "bread-compact-wifi-128x64");
    cJSON_AddStringToObject(board, "ssid", "hello");
    cJSON_AddNumberToObject(board, "rssi", -55);
    cJSON_AddNumberToObject(board, "channel", 1);
    cJSON_AddStringToObject(board, "ip", "192.168.54.39");
    cJSON_AddStringToObject(board, "mac", (char *)device_id);

    cJSON_AddItemToObject(root, "board", board);

    // 获取对应的json字符串
    char *json = cJSON_PrintUnformatted(root);

    MyLogE("%s", json);

    // 添加到请求体
    Driver_HttpClient_SetBody(json, strlen(json));
    // 注意: 不要在没有发请求之前 就回收掉json的内存
    Driver_HttpClient_Request();
    // 回收内存
    cJSON_Delete(root);
    free(json);
}

void generate_uuid_v4(char *out_str) {
    uint8_t uuid[16];
    
    // 1. 获取硬件生成的随机原始字节
    esp_fill_random(uuid, 16);

    // 2. 根据 RFC 4122 修改特定位
    // 设置版本号为 4 (0100xxxx)
    uuid[6] = (uuid[6] & 0x0F) | 0x40;
    // 设置变体为 RFC 4122 (10xxxxxx)
    uuid[8] = (uuid[8] & 0x3F) | 0x80;

    // 3. 格式化输出为 8-4-4-4-12 格式
    sprintf(out_str, 
            "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            uuid[0], uuid[1], uuid[2], uuid[3],
            uuid[4], uuid[5],
            uuid[6], uuid[7],
            uuid[8], uuid[9],
            uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
}