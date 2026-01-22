#include "App_Display.h"
#include "font_awesome.h"
#include "font_emoji.h"
#include "font_puhui.h"

lv_display_t *lvgl_disp;
lv_obj_t *scr;
lv_obj_t *title;
lv_obj_t *contentLabel;
lv_obj_t *contentCont; // 新增内容容器
lv_obj_t *emojiLable;
lv_obj_t *wifiIcon;    // 新增 WiFi 图标

typedef struct
{
    char *text;
    char *emoji;
} EmojiStruct;

EmojiStruct emojiList[] = {
    {"neutral", "😶"},
    {"happy", "🙂"},
    {"laughing", "😆"},
    {"funny", "😂"},
    {"sad", "😔"},
    {"angry", "😠"},
    {"crying", "😭"},
    {"loving", "😍"},
    {"embarrassed", "😳"},
    {"surprised", "😯"},
    {"shocked", "😱"},
    {"thinking", "🤔"},
    {"winking", "😉"},
    {"cool", "😎"},
    {"relaxed", "😌"},
    {"delicious", "🤤"},
    {"kissy", "😘"},
    {"confident", "😏"},
    {"sleepy", "😴"},
    {"silly", "😜"},
    {"confused", "🙄"}};

// 声明中文字体
LV_FONT_DECLARE(font_puhui_16_4);
static void App_Display_LvglInit(void)
{
    /* Initialize LVGL */
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,                  /* LVGL task priority */
        .task_stack = 10*1024,                  /* LVGL task stack size */
        .task_affinity = -1,                 /* LVGL task pinned to core (-1 is no affinity) */
        .task_max_sleep_ms = 500,            /* Maximum sleep in LVGL task */
        .timer_period_ms = 5,                /* LVGL timer tick period in ms */
        .task_stack_caps = MALLOC_CAP_SPIRAM // lvgl的任务堆栈放再哪里
    };
    lvgl_port_init(&lvgl_cfg);

    // 添加LCD屏幕
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = LCD_H_RES * 10,          // 缓冲大小
        .double_buffer = false,                 // 是否使用双缓冲
        .hres = LCD_H_RES,                      // 屏幕宽度
        .vres = LCD_V_RES,                      // 高度
        .monochrome = false,                    // 是否黑白色
        .color_format = LV_COLOR_FORMAT_RGB565, // 每个像素颜色格式
        .rotation = {
            .swap_xy = false, // 这三个要和lcd的配置一致
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,
            .swap_bytes = true,    // 是否交换颜色的字节顺序[MCU是小端序 SPI是大端序]
            .full_refresh = false, // 是否整屏刷新
            .direct_mode = false,  // 是否需要整屏缓冲
            .buff_spiram = true    // 缓冲使用外部内存

        }};
    lvgl_disp = lvgl_port_add_disp(&disp_cfg);
}

/**
 * @brief 透明度动画回调函数
 */
static void App_Display_OpaAnimCb(void * obj, int32_t v)
{
    lv_obj_set_style_opa((lv_obj_t *)obj, (lv_opa_t)v, 0);
}

/**
 * @brief 创建需要的lvgl组件
 *
 */
static void App_Display_CreateCompent(void)
{

    // 获取一个活动的屏幕
    scr = lv_scr_act();
    lvgl_port_lock(0);

    // 设置屏幕背景色为深色，更有科技感
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x121212), 0);

    /*------------------创建标题------------------------*/
    title = lv_label_create(scr);
    lv_obj_set_size(title, lv_pct(100), 32);
    lv_obj_set_style_bg_opa(title, LV_OPA_COVER, 0);
    // 使用深蓝色渐变背景
    lv_obj_set_style_bg_color(title, lv_color_hex(0x1A73E8), 0);
    lv_obj_set_style_bg_grad_color(title, lv_color_hex(0x0D47A1), 0);
    lv_obj_set_style_bg_grad_dir(title, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_radius(title, 0, 0); // 顶部平齐
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_label_set_text(title, "启动中...");
    lv_obj_set_style_text_font(title, &font_puhui_16_4, 0);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_pad_top(title, 6, 0); // 微调文字垂直居中

    /*-----------------------创建内容容器（气泡卡片）---------------------------*/
    contentCont = lv_obj_create(scr);
    lv_obj_set_size(contentCont, lv_pct(92), 100); // 稍微加高容器
    lv_obj_align(contentCont, LV_ALIGN_BOTTOM_MID, 0, -10); // 靠下放置
    lv_obj_set_style_bg_color(contentCont, lv_color_hex(0x212121), 0); // 更深的背景色
    lv_obj_set_style_radius(contentCont, 12, 0);
    lv_obj_set_style_border_width(contentCont, 0, 0);
    lv_obj_set_style_shadow_width(contentCont, 10, 0);
    lv_obj_set_style_shadow_opa(contentCont, LV_OPA_30, 0);
    // 关键优化：增加内边距，防止文字贴边
    lv_obj_set_style_pad_all(contentCont, 12, 0);
    // 允许内容滚动
    lv_obj_set_scrollbar_mode(contentCont, LV_SCROLLBAR_MODE_AUTO);

    contentLabel = lv_label_create(contentCont);
    lv_label_set_text(contentLabel, "");
    lv_obj_set_style_text_color(contentLabel, lv_color_hex(0xE0E0E0), 0);
    lv_obj_set_style_text_font(contentLabel, &font_puhui_16_4, 0);
    lv_obj_set_width(contentLabel, lv_pct(100));
    lv_label_set_long_mode(contentLabel, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_style_text_align(contentLabel, LV_TEXT_ALIGN_CENTER, 0);
    // 关键优化：增加行间距（Line Spacing）
    lv_obj_set_style_text_line_space(contentLabel, 4, 0);
    lv_obj_center(contentLabel);

    /*-----------------------创建Emoji标签--------------------------*/
    emojiLable = lv_label_create(scr);
    lv_obj_set_style_text_font(emojiLable, font_emoji_32_init(), 0);
    lv_label_set_text(emojiLable, "🙂");
    lv_obj_align(emojiLable, LV_ALIGN_CENTER, 0, -55);

    /*-----------------------创建WiFi图标--------------------------*/
    wifiIcon = lv_label_create(scr);
    lv_label_set_text(wifiIcon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(wifiIcon, lv_color_hex(0xAAAAAA), 0); // 默认灰色
    lv_obj_align(wifiIcon, LV_ALIGN_TOP_RIGHT, -10, 7);

    // 为 Emoji 增加呼吸动画
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, emojiLable);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_40);
    lv_anim_set_duration(&a, 1500);
    lv_anim_set_playback_duration(&a, 1500);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&a, App_Display_OpaAnimCb);
    lv_anim_start(&a);

    lvgl_port_unlock();
}

void App_Display_Init(void)
{
    Inf_Lcd_Init();

    App_Display_LvglInit();

    App_Display_CreateCompent();
}

/**
 * @brief 设置标题显示内容
 *
 * @param datas
 */
void App_Display_SetTitleText(char *datas)
{

    lvgl_port_lock(0);
    lv_label_set_text(title, datas);
    lvgl_port_unlock();
}

/**
 * @brief 设置内容标签显示文字
 *
 * @param datas
 */
void App_Display_SetContentText(char *datas)
{
    lvgl_port_lock(0);
    lv_label_set_text(contentLabel, datas);
    // 确保文字更新后，容器滚动回到最顶部
    lv_obj_scroll_to_y(contentCont, 0, LV_ANIM_OFF);
    lvgl_port_unlock();
}

/**
 * @brief 设置emoji显示文字
 *
 * @param datas
 */
void App_Display_SetEmojiText(char *emotion)
{

    // 从数组中遍历
    for (uint8_t i = 0; i < sizeof(emojiList) / sizeof(EmojiStruct); i++)
    {

        if (strcmp(emojiList[i].text, emotion) == 0)
        {
            lvgl_port_lock(0);
            lv_label_set_text(emojiLable, emojiList[i].emoji);
            lvgl_port_unlock();
            return;
        }
    }
}

/**
 * @brief 根据 RSSI 更新 WiFi 图标颜色
 * @param rssi 信号强度，0 表示断开
 */
void App_Display_SetWifiIcon(int rssi)
{
    lvgl_port_lock(0);
    if (rssi == 0) {
        lv_obj_set_style_text_color(wifiIcon, lv_color_hex(0xFF0000), 0); // 红色表示断开
        lv_label_set_text(wifiIcon, LV_SYMBOL_CLOSE);
    } else {
        lv_label_set_text(wifiIcon, LV_SYMBOL_WIFI);
        if (rssi > -50) {
            // 信号极好
            lv_obj_set_style_text_color(wifiIcon, lv_color_hex(0x00FF00), 0); // 绿色
        } else if (rssi > -70) {
            // 信号一般
            lv_obj_set_style_text_color(wifiIcon, lv_color_hex(0xFFFF00), 0); // 黄色
        } else {
            // 信号较差
            lv_obj_set_style_text_color(wifiIcon, lv_color_hex(0xFFA500), 0); // 橙色
        }
    }
    lvgl_port_unlock();
}

lv_obj_t * qr;
/**
 * @brief 显示二维码
 * 
 * @param datas 
 * @param len 
 */
void App_Display_ShowQRCode(void* datas, size_t len){

    lvgl_port_lock(0);
    //背景色
    lv_color_t bg_color = lv_color_white();
    //前景色
    lv_color_t fg_color = lv_color_black();

    qr = lv_qrcode_create(scr);
    lv_qrcode_set_size(qr, 200);
    lv_qrcode_set_dark_color(qr, fg_color);
    lv_qrcode_set_light_color(qr, bg_color);

    lv_qrcode_update(qr, datas, len);
    lv_obj_center(qr);

    /*Add a border with bg_color*/
    lv_obj_set_style_border_color(qr, bg_color, 0);
    lv_obj_set_style_border_width(qr, 8, 0);
    lvgl_port_unlock();
}

/**
 * @brief 删除二维码
 * 
 */
void App_Display_DeleteQRCode(void){

    if(qr){
        
        lvgl_port_lock(0);
        lv_obj_delete(qr);
        lvgl_port_unlock();
    }
}