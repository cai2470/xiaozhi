#ifndef __INF_IOT_H__
#define __INF_IOT_H__

// 获取设备描述符（静态字符串）
char *Inf_IOT_GetDescriptors(void);

// 获取设备状态（静态字符串）
char *Inf_IOT_GetStatus(void);

// 处理服务器下发的控制指令
void Inf_IOT_HandleCommand(char *json_str, int len);

#endif /* __INF_IOT_H__ */