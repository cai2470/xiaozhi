#include "App_Communication.h"

#define UPLOAD_AUDIO_TASK_NAME "upload"
#define UPLOAD_AUDIO_TASK_STACKDEPTH 4096
#define UPLOAD_AUDIO_TASK_PRIORITY 6
TaskHandle_t uploadHandle;

// 处理websocket接收的数据
static void App_Communication_WebsocketReceiveHandle(char *datas, int len, WebsocketDataType type);
// websocket正常断开连接回调
static void App_Communication_WebsocketFinishFunc(void);

// 上传用户音频给服务器
static void App_Communication_UploadAudioTaskFunc(void *args);

void App_Communication_Init(void)
{
    // 初始化websocket
    Driver_Websocket_Init(websocket_url);
    // 注册websocket回调
    Driver_Websocket_RegisterCallback(App_Communication_WebsocketReceiveHandle, App_Communication_WebsocketFinishFunc);

    // 创建一个上传音频的任务
    xTaskCreateWithCaps(App_Communication_UploadAudioTaskFunc, UPLOAD_AUDIO_TASK_NAME, UPLOAD_AUDIO_TASK_STACKDEPTH, NULL, UPLOAD_AUDIO_TASK_PRIORITY, &uploadHandle, MALLOC_CAP_SPIRAM);
}

/**
 * @brief 主动向服务器推送当前设备状态
 */
void App_Communication_PushStatus(void)
{
    if (session_id == NULL || !Driver_Websocket_IsConnected()) {
        MyLogW("PushStatus: 未连接或无 SessionID，跳过同步");
        return;
    }

    char *raw_status = Inf_IOT_GetStatus();
    if (raw_status == NULL) return;

    cJSON *status_json = cJSON_Parse(raw_status);
    if (status_json) {
        // 确保 session_id 注入成功
        if (!cJSON_HasObjectItem(status_json, "session_id")) {
            cJSON_AddStringToObject(status_json, "session_id", session_id);
        }
        
        char *final_status = cJSON_PrintUnformatted(status_json);
        if (final_status) {
            Driver_Websocket_Send(final_status, strlen(final_status), WEBSOCKET_TEXT_DATA);
            free(final_status);
        }
        cJSON_Delete(status_json);
    }
    free(raw_status); // 释放 Inf_IOT_GetStatus 分配的字符串内存
}

// 处理websocket接收的数据
static void App_Communication_WebsocketReceiveHandle(char *datas, int len, WebsocketDataType type)
{

    if (type == WEBSOCKET_TEXT_DATA)
    {
        // 收到的是文本数据
        MyLogE("收到数据:%.*s", len, datas);
        cJSON *root = cJSON_ParseWithLength(datas, len);

        if (root == NULL)
        {
            MyLogE("服务器返回的json格式存在问题..");
            return;
        }

        cJSON *type = cJSON_GetObjectItemCaseSensitive(root, "type");

        if (type == NULL || !cJSON_IsString(type))
        {
            MyLogE("type字段不存在或者类型错误");
            cJSON_Delete(root);
            return;
        }

        if (strcmp(type->valuestring, "hello") == 0)
        {
            // --- A. 获取并保存 Session ID ---
            cJSON *session = cJSON_GetObjectItemCaseSensitive(root, "session_id");
            if (session && cJSON_IsString(session))
            {
                if (session_id) {
                    free(session_id);
                    session_id = NULL;
                }
                session_id = strdup(session->valuestring); // 保存全局 session_id
                xEventGroupSetBits(global_event, WEBSOCKET_HELLO_RESPONSE);
            }

            if (session_id)
            {
                // --- B. 发送 Descriptors (关键：动态注入 session_id) ---
                // 1. 获取你在 Inf_IOT.c 里写的静态字符串
                char *static_desc = Inf_IOT_GetDescriptors();

                // 2. 解析成 JSON 对象以便修改
                cJSON *desc_json = cJSON_Parse(static_desc);
                if (desc_json)
                {
                    // 3. 动态注入 session_id
                    if (cJSON_HasObjectItem(desc_json, "session_id")) {
                        cJSON_ReplaceItemInObject(desc_json, "session_id", cJSON_CreateString(session_id));
                    } else {
                        cJSON_AddStringToObject(desc_json, "session_id", session_id);
                    }

                    // 4. 转回字符串发送
                    char *final_desc = cJSON_PrintUnformatted(desc_json);
                    if (final_desc && Driver_Websocket_IsConnected()) {
                        Driver_Websocket_Send(final_desc, strlen(final_desc), WEBSOCKET_TEXT_DATA);
                        free(final_desc);
                    }
                    cJSON_Delete(desc_json);
                }

                // --- C. 发送 Status ---
                App_Communication_PushStatus();
            }
        }

        // =================================================================
        // 2. 处理 IoT 指令：直接转交，不要自己解析！
        // =================================================================
        else if (strcmp(type->valuestring, "iot") == 0)
        {
            // 参照你发的 Inf_IOT.c，里面已经写好了 HandleCommand 函数
            MyLogI("收到 AI 控制指令: %.*s", len, datas);
            // 所以这里 App_Communication 只需要做一个“传令兵”
            Inf_IOT_HandleCommand(datas, len);
            // 指令执行完后，立即推送最新状态给服务器，确保 AI 同步
            App_Communication_PushStatus();
        }
        else if (strcmp(type->valuestring, "tts") == 0)
        {
            // text to speach
            cJSON *state = cJSON_GetObjectItemCaseSensitive(root, "state");
            if (state == NULL || !cJSON_IsString(state))
            {
                MyLogW("TTS 消息缺少 state 字段");
                cJSON_Delete(root);
                return;
            }

            if (strcmp(state->valuestring, "start") == 0)
            {
                MyLogE("小智说话中...");
                // 小智开始说话
                communicationStatus = SPEAKING;
                App_Display_SetTitleText("说话中...");
            }
            else if (strcmp(state->valuestring, "stop") == 0)
            {
                // 小智说完了
                MyLogE("小智说话完成...");
                communicationStatus = IDLE;
                App_Display_SetTitleText("聆听中...");
            }
            else if (strcmp(state->valuestring, "sentence_start") == 0)
            {

                cJSON *text = cJSON_GetObjectItemCaseSensitive(root, "text");

                if (text && cJSON_IsString(text))
                {

                    App_Display_SetContentText(text->valuestring);
                }
            }
        }
        else if (strcmp(type->valuestring, "llm") == 0)
        {
            // 获取emoji
            cJSON *emotion = cJSON_GetObjectItemCaseSensitive(root, "emotion");
            if (emotion && cJSON_IsString(emotion))
            {
                App_Display_SetEmojiText(emotion->valuestring);
            }
        }

        // 回收内存
        cJSON_Delete(root);
    }
    else
    {
        // 音频数据
        if (xRingbufferSend(ws_to_decoder_buff, datas, len, 0) == pdFALSE)
        {
            MyLogW("ws_to_decoder_buff 满，丢弃下行音频数据");
        }
    }
}
// websocket正常断开连接回调
static void App_Communication_WebsocketFinishFunc(void)
{
    is_wakup = false;
    // 让状态回到空闲,等到下次说唤醒词的时候 重新连接服务器
    communicationStatus = IDLE;
}

/**
 * @brief 判断websoket是否连接
 *
 * @return true
 * @return false
 */
bool App_Communication_IsConnected(void)
{
    return Driver_Websocket_IsConnected();
}

/**
 * @brief 连接服务器
 *
 */
void App_Communication_ConnectServer(void)
{

    static bool is_first_connect = true;

    // 只有在client第一次连接的时候 才需要设置头信息 否则重复设置会报错
    if (is_first_connect)
    {

        // 设置头信息
        char *auth = NULL;
        asprintf(&auth, "Bearer %s", token);
        Driver_Websocket_AppendHeader("Authorization", auth);
        Driver_Websocket_AppendHeader("Protocol-Version", "1");
        Driver_Websocket_AppendHeader("Device-Id", (char *)device_id);
        Driver_Websocket_AppendHeader("Client-Id", (char *)client_id);

        is_first_connect = false;
    }

    // 启动websocket连接服务器
    Driver_Websocket_Start();

    // 等待服务器连接成功
    xEventGroupWaitBits(global_event, WEBSOCKET_CONNECT_SUCCESS, pdTRUE, pdTRUE, portMAX_DELAY);

    MyLogE("服务器连接成功...");
}

/**
 * @brief 发送hello
 *
 */
void App_Communication_SendHello(void)
{

    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "type", "hello");
    cJSON_AddNumberToObject(root, "version", 1);
    cJSON_AddStringToObject(root, "transport", "websocket");

    cJSON *features = cJSON_CreateObject();
    // 关键点：开启 MCP 支持，允许 AI 将设备视为“工具”进行调用
    cJSON_AddBoolToObject(features, "mcp", true);
    cJSON_AddItemToObject(root, "features", features);

    cJSON *audio_params = cJSON_CreateObject();
    cJSON_AddStringToObject(audio_params, "format", "opus");
    cJSON_AddNumberToObject(audio_params, "sample_rate", 16000);
    cJSON_AddNumberToObject(audio_params, "channels", 1);
    cJSON_AddNumberToObject(audio_params, "frame_duration", 60);
    cJSON_AddItemToObject(root, "audio_params", audio_params);

    char *json = cJSON_PrintUnformatted(root);

    // 发送
    Driver_Websocket_Send(json, strlen(json), WEBSOCKET_TEXT_DATA);

    // 回收内存
    cJSON_Delete(root);
    free(json);

    // 等待回应
    xEventGroupWaitBits(global_event, WEBSOCKET_HELLO_RESPONSE, pdTRUE, pdTRUE, portMAX_DELAY);
}

/**
 * @brief 发送唤醒词
 *
 */
void App_Communication_SendWakup(void)
{

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "session_id", session_id);
    cJSON_AddStringToObject(root, "type", "listen");
    cJSON_AddStringToObject(root, "state", "detect");
    cJSON_AddStringToObject(root, "text", "你好,小智");

    char *json = cJSON_PrintUnformatted(root);

    Driver_Websocket_Send(json, strlen(json), WEBSOCKET_TEXT_DATA);

    cJSON_Delete(root);
    free(json);
}

/**
 * @brief 开始监听
 *
 */
void App_Communication_StartListing(void)
{

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "session_id", session_id);
    cJSON_AddStringToObject(root, "type", "listen");
    cJSON_AddStringToObject(root, "state", "start");
    cJSON_AddStringToObject(root, "mode", "manual");

    char *json = cJSON_PrintUnformatted(root);

    // 发出
    Driver_Websocket_Send(json, strlen(json), WEBSOCKET_TEXT_DATA);

    cJSON_Delete(root);
    free(json);
}

/**
 * @brief 停止监听
 *
 */
void App_Communication_StopListing(void)
{

    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "session_id", session_id);
    cJSON_AddStringToObject(root, "type", "listen");
    cJSON_AddStringToObject(root, "state", "stop");

    char *json = cJSON_PrintUnformatted(root);

    Driver_Websocket_Send(json, strlen(json), WEBSOCKET_TEXT_DATA);

    cJSON_Delete(root);
    free(json);
}

// 上传用户音频给服务器
static void App_Communication_UploadAudioTaskFunc(void *args)
{

    MyLogE("上传用户音频到服务器任务启动...");

    size_t size = 0;
    char *datas = NULL;
    while (1)
    {

        // 取出音频数据
        datas = (char *)xRingbufferReceive(encoder_to_ws_buff, &size, portMAX_DELAY);

        if (datas != NULL) {
            //  只有在监听的时候 才需要发送音频
            if (session_id && communicationStatus == LISTING && size > 0)
            {
                // 将音频发到服务器
                Driver_Websocket_Send(datas, (int)size, WEBSOCKET_BIN_DATA);
            }
            // 内存回收
            vRingbufferReturnItem(encoder_to_ws_buff, datas);
        }
        size = 0; // 确保每次循环开始前 size 状态明确

    }
}

/**
 * @brief 让服务器放弃说话
 *
 */
void App_Communication_Abort(void)
{

    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "session_id", session_id);
    cJSON_AddStringToObject(root, "type", "abort");

    char *json = cJSON_PrintUnformatted(root);

    Driver_Websocket_Send(json, strlen(json), WEBSOCKET_TEXT_DATA);

    cJSON_Delete(root);
    free(json);
}