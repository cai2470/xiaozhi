#include "Inf_Led.h"

#define LED_STRIP_GPIO_PIN 46
#define LED_STRIP_LED_COUNT 2

#define LED_STRIP_RMT_RES_HZ (10 * 1000 * 1000)
#define LED_STRIP_MEMORY_BLOCK_WORDS 0
#define LED_STRIP_USE_DMA 0

led_strip_handle_t led_strip;
static bool s_led_is_open = false;
static int s_r = 0, s_g = 0, s_b = 0;

void Inf_Led_Init(void)
{
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_GPIO_PIN,                        // The GPIO that connected to the LED strip's data line
        .max_leds = LED_STRIP_LED_COUNT,                             // The number of LEDs in the strip,
        .led_model = LED_MODEL_WS2812,                               // LED strip model
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_RGB, // The color order of the strip: GRB
        .flags = {
            .invert_out = false, // don't invert the output signal
        }};

    // LED strip backend configuration: RMT
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,                    // different clock source can lead to different power consumption
        .resolution_hz = LED_STRIP_RMT_RES_HZ,             // RMT counter clock frequency
        .mem_block_symbols = LED_STRIP_MEMORY_BLOCK_WORDS, // the memory block size used by the RMT channel
        .flags = {
            .with_dma = LED_STRIP_USE_DMA, // Using DMA can improve performance when driving more LEDs
        }};

    // LED Strip object handle

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
}

void Inf_Led_Open(void)
{

    for (int i = 0; i < LED_STRIP_LED_COUNT; i++)
    {
        ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, 240, 240, 240));
    }
    
    ESP_ERROR_CHECK(led_strip_refresh(led_strip));
    s_led_is_open = true;
    s_r = 240;
    s_g = 240;
    s_b = 240;
}

void Inf_Led_Close(void)
{
    led_strip_clear(led_strip);
    s_led_is_open = false;
}



// 【新增】设置颜色函数
void Inf_Led_SetColor(int r, int g, int b)
{
    // 遍历所有灯珠，设置颜色
    for (int i = 0; i < LED_STRIP_LED_COUNT; i++)
    {
        ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, r, g, b));
    }
    // 刷新显示
    ESP_ERROR_CHECK(led_strip_refresh(led_strip));
    s_led_is_open = true;
    s_r = r;
    s_g = g;
    s_b = b;
}

// 【新增】获取灯光状态
bool Inf_Led_Is_Open(void)
{
    return s_led_is_open;
}

void Inf_Led_GetColor(int *r, int *g, int *b)
{
    if (r) *r = s_r;
    if (g) *g = s_g;
    if (b) *b = s_b;
}
