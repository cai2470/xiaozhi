#include "Inf_IOT.h"
#include "cJSON.h"
#include "Inf_Led.h"       // 必须引用，用于控制硬件
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
            "\"TurnOn\":{\"description\":\"Turn on the light\"},"
            "\"TurnOff\":{\"description\":\"Turn off the light\"},"
            "\"SetColor\":{\"description\":\"Set RGB color\",\"parameters\":{"
                "\"r\":{\"description\":\"Red[0-255]\",\"type\":\"number\"},"
                "\"g\":{\"description\":\"Green[0-255]\",\"type\":\"number\"},"
                "\"b\":{\"description\":\"Blue[0-255]\",\"type\":\"number\"}"
            "}}"
        "}}"
    "]}";

// 注意：静态状态字符串意味着状态永远是 off，
// 如果你想让状态实时变化，这里以后需要改成动态生成的。
// 目前为了跑通逻辑，先保持静态。
static const char *iot_status =
    "{\"type\":\"iot\",\"status\":[{\"name\":\"Light\",\"state\":\"off\"}]}";

// ==========================================
// 2. 接口实现
// ==========================================

char *Inf_IOT_GetDescriptors(void)
{
    return (char *)descriptors;
}

char *Inf_IOT_GetStatus(void)
{
    return (char *)iot_status;
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
    cJSON *commands = cJSON_GetObjectItem(root, "commands");
    if (commands)
    {
        // 获取第一条指令
        cJSON *item = cJSON_GetArrayItem(commands, 0);
        if (item)
        {
            // 获取方法名
            cJSON *method = cJSON_GetObjectItem(item, "method");
            // 获取参数
            cJSON *params = cJSON_GetObjectItem(item, "parameters");

            if (method && method->valuestring)
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
                        cJSON *r = cJSON_GetObjectItem(params, "r");
                        cJSON *g = cJSON_GetObjectItem(params, "g");
                        cJSON *b = cJSON_GetObjectItem(params, "b");

                        if (r && g && b)
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
            }
        }
    }
    cJSON_Delete(root);
}