/**
 * @file Driver_WIFI.c
 * @brief 实现 Wi-Fi 配网 (回调风格 + 鲁棒性优化)
 */

#include "sdkconfig.h" // 必须放在第一行
#include <stdio.h>
#include <string.h>
#include "Driver_WIFI.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"

/* 配网管理器组件 */
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_ble.h"

/* 二维码生成组件 */
#include "qrcode.h"

static const char *TAG = "DRV_WIFI";

/* --- 配置参数 --- */
#define PROV_SECURITY_VER WIFI_PROV_SECURITY_1
#define PROV_POP "123456"           // 你的配网密码
#define PROV_SERVICE_PREFIX "PROV_" // 蓝牙名称前缀
#define WIFI_MAX_RETRY 100            // 最大重连次数



// Wi-Fi 连接成功后的回调函数类型
typedef void (*WifiConnectedCallback)(void);

// 二维码生成后的回调函数类型 (会将 payload 字符串传出去)
typedef void (*QrCodeCallback)(const char *payload);

/* --- 静态变量 (保存用户的回调函数) --- */
static WifiConnectedCallback s_connect_cb = NULL;
static QrCodeCallback s_qrcode_cb = NULL;
static int s_retry_num = 0;

/* --- 内部辅助函数声明 --- */
static void wifi_init_sta(void);
static void get_device_service_name(char *service_name, size_t max);
static void print_qr(const char *name, const char *pop, const char *transport);

/* * ===============================================================
 * 事件处理核心逻辑
 * ===============================================================
 */
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    /* 1. Wi-Fi 基础事件 */
    if (event_base == WIFI_EVENT)
    {
        if (event_id == WIFI_EVENT_STA_START)
        {
            esp_wifi_connect();
        }
        else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
        {
            /* 优化：带有重试机制的重连，防止无限死循环 */
            if (s_retry_num < WIFI_MAX_RETRY)
            {
                esp_wifi_connect();
                s_retry_num++;
                ESP_LOGW(TAG, "Wi-Fi disconnected, retrying... (%d/%d)", s_retry_num, WIFI_MAX_RETRY);
            }
            else
            {
                ESP_LOGE(TAG, "Failed to connect to Wi-Fi. Check password or Router.");
                // 这里可以扩展：比如调用回调通知 App 层连接失败
            }
        }
    }
    /* 2. IP 获取事件 (连接成功) */
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Connected with IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;

        /* 核心：触发用户注册的回调函数 */
        if (s_connect_cb)
        {
            ESP_LOGI(TAG, "Triggering User Callback...");
            s_connect_cb();
        }
    }
    /* 3. 配网相关事件 */
    else if (event_base == WIFI_PROV_EVENT)
    {
        switch (event_id)
        {
        case WIFI_PROV_START:
            ESP_LOGI(TAG, "Provisioning started");
            break;
        case WIFI_PROV_CRED_RECV:
        {
            wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
            ESP_LOGI(TAG, "Received Credentials -> SSID: %s", (const char *)wifi_sta_cfg->ssid);
            break;
        }
        case WIFI_PROV_CRED_FAIL:
        {
            wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
            ESP_LOGE(TAG, "Provisioning Failed: %s",
                     (*reason == WIFI_PROV_STA_AUTH_ERROR) ? "Auth Error" : "AP Not Found");
            /* 允许用户重试，而不是死锁 */
            wifi_prov_mgr_reset_sm_state_on_failure();
            break;
        }
        case WIFI_PROV_CRED_SUCCESS:
            ESP_LOGI(TAG, "Provisioning Successful");
            break;
        case WIFI_PROV_END:
            wifi_prov_mgr_deinit();
            break;
        default:
            break;
        }
    }
}

/* * ===============================================================
 * 辅助功能函数
 * ===============================================================
 */

/* 手动构建并打印/回调二维码 */
static void print_qr(const char *name, const char *pop, const char *transport)
{
    if (!name || !transport)
        return;

    char payload[150] = {0};
    /* 修复：使用 snprintf 替代隐式调用的 wifi_prov_mgr_get... */
    snprintf(payload, sizeof(payload),
             "{\"ver\":\"%s\",\"name\":\"%s\",\"pop\":\"%s\",\"transport\":\"%s\"}",
             "v1", name, pop, transport);

    ESP_LOGI(TAG, "Scan this QR code:");

    /* 1. 打印到 Log 终端 */
    esp_qrcode_config_t config = ESP_QRCODE_CONFIG_DEFAULT();
    esp_qrcode_generate(&config, payload);

    /* 2. 如果注册了回调 (比如要显示在屏幕上)，传出去 */
    if (s_qrcode_cb)
    {
        s_qrcode_cb(payload);
    }
}

static void get_device_service_name(char *service_name, size_t max)
{
    uint8_t eth_mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    snprintf(service_name, max, "%s%02X%02X%02X",
             PROV_SERVICE_PREFIX, eth_mac[3], eth_mac[4], eth_mac[5]);
}

static void wifi_init_sta(void)
{
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

/* * ===============================================================
 * 外部接口实现
 * ===============================================================
 */

void Driver_WIFI_Init(void)
{
    /* 1. NVS Init */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    /* 2. Netif & Event Loop */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    /* 3. Wi-Fi Init */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* 4. Register Handlers */
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));

    /* 5. Provisioning Manager Init */
    wifi_prov_mgr_config_t config = {
        .scheme = wifi_prov_scheme_ble,
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM};
    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));

    /* 6. Check status */
    bool provisioned = false;
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));

    if (!provisioned)
    {
        ESP_LOGI(TAG, "Starting BLE Provisioning...");

        char service_name[12];
        get_device_service_name(service_name, sizeof(service_name));

        wifi_prov_security_t security = PROV_SECURITY_VER;
        const char *pop = PROV_POP;

        /* 启动配网 */
        ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, pop, service_name, NULL));

        /* 处理二维码 (打印 + 回调) */
        print_qr(service_name, pop, "ble");
    }
    else
    {
        ESP_LOGI(TAG, "Already provisioned. Starting Wi-Fi...");
        wifi_prov_mgr_deinit();
        wifi_init_sta();
    }
}

void Driver_WIFI_Reset_Provisioning(void)
{
    wifi_prov_mgr_reset_provisioning();
    esp_restart();
}

void Driver_WIFI_RegisterCallback(WifiConnectedCallback cb)
{
    s_connect_cb = cb;
}

void Driver_WIFI_RegisterShowQrCodeCallback(QrCodeCallback cb)
{
    s_qrcode_cb = cb;
}