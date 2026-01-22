#include "Inf_IOT.h"
#include "cJSON.h"
#include "Inf_Led.h"       // 必须引用，用于控制硬件
#include "Inf_ES8311.h"    // 引用音频驱动以控制音量
#include "Common_Debug.h"  // 用于打印日志

// ==========================================
// 1. 静态描述符定义 (你的原始代码)
// ==========================================
static const char *descriptors =
    "{\"type\":\"iot\","
    "\"session_id\":\"\","  // 这里留空，会在 App_Communication.c 里动态填入
    "\"update\":true,"
    "\"descriptors\":["
        "{\"name\":\"Light\","
        "\"description\":\"RGB LED\","
        "\"properties\":{"
            "\"power\":{\"description\":\"Power state\",\"type\":\"boolean\"},"
            "\"r\":{\"description\":\"Red value\",\"type\":\"number\"},"
            "\"g\":{\"description\":\"Green value\",\"type\":\"number\"},"
            "\"b\":{\"description\":\"Blue value\",\"type\":\"number\"}"
        "},"
        "\"methods\":{"
            "\"TurnOn\":{\"description\":\"Turn on the RGB LED light to white color\"},"
            "\"TurnOff\":{\"description\":\"Turn off the RGB LED light completely\"},"
            "\"SetColor\":{\"description\":\"Set RGB color\",\"parameters\":{"
                "\"r\":{\"description\":\"Red[0-255]\",\"type\":\"number\"},"
                "\"g\":{\"description\":\"Green[0-255]\",\"type\":\"number\"},"
                "\"b\":{\"description\":\"Blue[0-255]\",\"type\":\"number\"}"
            "}}"
        "}},"
        "{\"name\":\"Speaker\","
        "\"description\":\"System Speaker\","
        "\"properties\":{"
            "\"volume\":{\"description\":\"Current volume\",\"type\":\"number\"}"
        "},"
        "\"methods\":{"
            "\"SetVolume\":{\"description\":\"Set speaker volume\",\"parameters\":{"
                "\"volume\":{\"description\":\"Volume level [0-100]\",\"type\":\"number\"}"
            "}}"
        "}}"
    "]}";

// ==========================================
// 2. 接口实现
// ==========================================

char *Inf_IOT_GetDescriptors(void)
{
    return (char *)descriptors;
}

char *Inf_IOT_GetStatus(void)
{
    MyLogI("正在获取设备实时状态以同步至 AI 服务器...");

    // 改进点：动态生成状态，让 AI 知道灯到底是开还是关
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "iot");
    
    cJSON *status_array = cJSON_CreateArray();
    cJSON *light_status = cJSON_CreateObject();
    cJSON_AddStringToObject(light_status, "name", "Light");

    int r, g, b;
    Inf_Led_GetColor(&r, &g, &b);

    // 根据硬件实际状态返回 "on" 或 "off"
    cJSON_AddStringToObject(light_status, "state", Inf_Led_Is_Open() ? "on" : "off");
    cJSON_AddNumberToObject(light_status, "r", r);
    cJSON_AddNumberToObject(light_status, "g", g);
    cJSON_AddNumberToObject(light_status, "b", b);
    
    // 新增：上报音量状态
    cJSON *speaker_status = cJSON_CreateObject();
    cJSON_AddStringToObject(speaker_status, "name", "Speaker");
    cJSON_AddNumberToObject(speaker_status, "volume", Inf_ES8311_GetVolume());
    cJSON_AddItemToArray(status_array, speaker_status);

    cJSON_AddItemToArray(status_array, light_status);
    cJSON_AddItemToObject(root, "status", status_array);
    
    char *status_str = cJSON_PrintUnformatted(root);
    // 注意：这里返回的字符串需要在调用处释放，或者使用静态缓冲区
    // 为了兼容你现有的 App_Communication.c 逻辑，建议在 App_Communication 里处理释放
    cJSON_Delete(root);
    return status_str;
}

/**
 * @brief 处理 IoT 控制指令
 * 解析 TurnOn, TurnOff, SetColor 并调用 Inf_Led 驱动
 */
void Inf_IOT_HandleCommand(char *json_str, int len)
{
    cJSON *root = cJSON_ParseWithLength(json_str, len);
    if (!root) {
        MyLogE("IoT指令解析失败");
        return;
    }

    // 获取 commands 数组
    cJSON *commands = cJSON_GetObjectItemCaseSensitive(root, "commands");
    if (cJSON_IsArray(commands) && cJSON_GetArraySize(commands) > 0)
    {
        // 获取第一条指令
        cJSON *item = cJSON_GetArrayItem(commands, 0);
        if (item)
        {
            // 获取方法名
            cJSON *method = cJSON_GetObjectItemCaseSensitive(item, "method");
            // 获取参数
            cJSON *params = cJSON_GetObjectItemCaseSensitive(item, "parameters");

            if (cJSON_IsString(method) && method->valuestring)
            {
                // --- A: 开灯 ---
                if (strcmp(method->valuestring, "TurnOn") == 0)
                {
                    MyLogE(">>> 执行指令: 开灯 <<<");
                    Inf_Led_Open(); // 调用 LED 驱动
                }
                // --- B: 关灯 ---
                else if (strcmp(method->valuestring, "TurnOff") == 0)
                {
                    MyLogE(">>> 执行指令: 关灯 <<<");
                    Inf_Led_Close(); // 调用 LED 驱动
                }
                // --- C: 设置颜色 ---
                else if (strcmp(method->valuestring, "SetColor") == 0)
                {
                    if (params)
                    {
                        cJSON *r = cJSON_GetObjectItemCaseSensitive(params, "r");
                        cJSON *g = cJSON_GetObjectItemCaseSensitive(params, "g");
                        cJSON *b = cJSON_GetObjectItemCaseSensitive(params, "b");

                        if (cJSON_IsNumber(r) && cJSON_IsNumber(g) && cJSON_IsNumber(b))
                        {
                            int red = r->valueint;
                            int green = g->valueint;
                            int blue = b->valueint;

                            MyLogE(">>> 执行指令: 变色 (%d,%d,%d) <<<", red, green, blue);
                            
                            // 调用 LED 变色驱动 (需要在 Inf_Led.h 里声明)
                            Inf_Led_SetColor(red, green, blue);
                        }
                    }
                }
                // --- D: 设置音量 ---
                else if (strcmp(method->valuestring, "SetVolume") == 0)
                {
                    if (params)
                    {
                        cJSON *vol = cJSON_GetObjectItemCaseSensitive(params, "volume");
                        if (cJSON_IsNumber(vol))
                        {
                            int volume = vol->valueint;
                            MyLogE(">>> 执行指令: 设置音量 (%d) <<<", volume);
                            Inf_ES8311_SetVolume(volume);
                        }
                    }
                }
            }
        }
    }
    cJSON_Delete(root);
}