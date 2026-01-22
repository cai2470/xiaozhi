/**
* @file Driver_WIFI.h
 * @brief Wi-Fi 驱动接口 (回调函数风格)
 */

#ifndef DRIVER_WIFI_H
#define DRIVER_WIFI_H

#include "esp_err.h"

/* * 1. 加上 extern "C" 是为了保证 C++ 编译器能正确调用这些 C 函数
 * (ESP-IDF 底层很多是 C++ 混合编译的，这个不能少)
 */
#ifdef __cplusplus
extern "C" {
#endif

  /* * 2. 必须找回这两个 typedef！
   * 没有它们，下面的函数声明里的参数类型就是未定义的，编译器会报错
   */
  typedef void (*WifiConnectedCallback)(void);
  typedef void (*QrCodeCallback)(const char *payload);

  /* --- 3. 外部接口 --- */

  /**
   * @brief 初始化 Wi-Fi 驱动
   */
  void Driver_WIFI_Init(void);

  /**
   * @brief 注册 Wi-Fi 连接成功的回调
   */
  void Driver_WIFI_RegisterCallback(WifiConnectedCallback cb);

  /**
   * @brief 注册二维码显示回调
   */
  void Driver_WIFI_RegisterShowQrCodeCallback(QrCodeCallback cb);

  int Driver_WIFI_GetRSSI(void);

  /**
   * @brief 重置配网信息并重启
   */
  void Driver_WIFI_Reset_Provisioning(void);

  /* * 4. 这里的闭合括号必须有，和上面的 { 对应
   */
#ifdef __cplusplus
}
#endif

#endif // DRIVER_WIFI_H