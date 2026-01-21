#include "Driver_Websocket.h"
#include "esp_log.h"

static const char *WS_TAG = "websocket";

WebsocketReceiveHandleFunc receiveHandleCallback;
WebsocketFinishHandleFunc finishCallback;
static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (event_id)
    {
    case WEBSOCKET_EVENT_BEGIN:
        ESP_LOGI(WS_TAG, "WEBSOCKET_EVENT_BEGIN");
        break;
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(WS_TAG, "WEBSOCKET_EVENT_CONNECTED");
        xEventGroupSetBits(global_event,WEBSOCKET_CONNECT_SUCCESS);
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGI(WS_TAG, "WEBSOCKET_EVENT_DISCONNECTED");

        break;
    case WEBSOCKET_EVENT_DATA:
        //ESP_LOGI(WS_TAG, "WEBSOCKET_EVENT_DATA");
        //ESP_LOGI(WS_TAG, "Received opcode=%d", data->op_code);
        if (receiveHandleCallback)
        {

            if (data->op_code == 0x01)
            {
                // 收到服务器发来的文本数据
                receiveHandleCallback(data->data_ptr, data->data_len, WEBSOCKET_TEXT_DATA);
            }
            else if (data->op_code == 0x02)
            {
                // 收到服务器发来的二进制数据
                receiveHandleCallback(data->data_ptr, data->data_len, WEBSOCKET_BIN_DATA);
            }
        }
        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGI(WS_TAG, "WEBSOCKET_EVENT_ERROR");

        break;
    case WEBSOCKET_EVENT_FINISH:
        ESP_LOGI(WS_TAG, "WEBSOCKET_EVENT_FINISH");
        if (finishCallback)
        {
            finishCallback();
        }
        break;
    }
}

esp_websocket_client_handle_t wsClient;

void Driver_Websocket_Init(char *url)
{

    esp_websocket_client_config_t websocket_cfg = {
        .uri = url,
        .disable_auto_reconnect = false, // 开启自动重连
        .buffer_size = 3 * 1024,
        .transport = WEBSOCKET_TRANSPORT_OVER_SSL,  // 使用安全的websocket
        .crt_bundle_attach = esp_crt_bundle_attach, // 绑定证书
        .reconnect_timeout_ms = 3000,
        .network_timeout_ms = 3000,
    };
    // 初始化websocket客户端
    wsClient = esp_websocket_client_init(&websocket_cfg);
    // 注册事件回调
    esp_websocket_register_events(wsClient, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)wsClient);
}

/**
 * @brief 启动websocket
 *
 */
void Driver_Websocket_Start(void)
{
    if (wsClient)
    {
        esp_websocket_client_start(wsClient);
    }
}

/**
 * @brief 启动websocket
 *
 */
void Driver_Websocket_Stop(void)
{
    if (wsClient)
    {
        esp_websocket_client_stop(wsClient);
    }
}

/**
 * @brief 判断websocket是否已经连接服务器
 *
 * @return true
 * @return false
 */
bool Driver_Websocket_IsConnected(void)
{
    if (wsClient)
    {
        return esp_websocket_client_is_connected(wsClient);
    }
    return false;
}

/**
 * @brief 发送数据
 *
 * @param datas
 * @param len
 * @param type
 */
void Driver_Websocket_Send(char *datas, int len, WebsocketDataType type)
{
    if (wsClient && datas && len)
    {

        if (type == WEBSOCKET_BIN_DATA)
        {
            esp_websocket_client_send_bin(wsClient, datas, len, 10);
        }
        else
        {
            esp_websocket_client_send_text(wsClient, datas, len, 10);
        }
    }
}

/**
 * @brief 追加请求头
 *
 * @param key
 * @param value
 */
void Driver_Websocket_AppendHeader(char *key, char *value)
{
    esp_websocket_client_append_header(wsClient, key, value);
}

/**
 * @brief 注册接收数据处理回调和websocket正常断开回调
 *
 * @param rcb
 * @param fcb
 */
void Driver_Websocket_RegisterCallback(WebsocketReceiveHandleFunc rcb, WebsocketFinishHandleFunc fcb)
{
    receiveHandleCallback = rcb;
    finishCallback = fcb;
}