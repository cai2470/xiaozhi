#include "App_Display.h"
#include "font_awesome.h"
#include "font_emoji.h"
#include "font_puhui.h"

lv_display_t *lvgl_disp;
lv_obj_t *scr;
lv_obj_t *title;
lv_obj_t *contentLabel;
lv_obj_t *emojiLable;

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
 * @brief 创建需要的lvgl组件
 *
 */
static void App_Display_CreateCompent(void)
{

    // 获取一个活动的屏幕
    scr = lv_scr_act();
    lvgl_port_lock(0);
    /*------------------创建标题------------------------*/
    // 创建一个标签
    title = lv_label_create(scr);
    // 设置标签的大小
    lv_obj_set_size(title, lv_pct(100), lv_pct(10));
    // 设置背景透明度
    lv_obj_set_style_bg_opa(title, LV_OPA_COVER, 0);
    // 设置背景色
    lv_obj_set_style_bg_color(title, lv_palette_lighten(LV_PALETTE_GREY, 1), 0);
    // 设置文字
    lv_label_set_text(title, "启动中...");
    // 设置文字的字体
    lv_obj_set_style_text_font(title, &font_puhui_16_4, 0);
    // 设置文字的对齐方式
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    // 设置对齐方式
    lv_obj_set_align(title, LV_ALIGN_TOP_MID);

    /*-----------------------创建内容标签---------------------------*/
    contentLabel = lv_label_create(scr);
    // 设置文字
    lv_label_set_text(contentLabel, "");
    // 设置文字的字体
    lv_obj_set_style_text_font(contentLabel, &font_puhui_16_4, 0);
    // 设置对齐方式
    lv_obj_align(contentLabel, LV_ALIGN_CENTER,0,0);
    //设置大小
    lv_obj_set_size(contentLabel, lv_pct(100), lv_pct(30));
    // 设置长文本模式[文本过长换行]
    lv_label_set_long_mode(contentLabel, LV_LABEL_LONG_MODE_WRAP);
    // 设置文字的对齐方式
    lv_obj_set_style_text_align(contentLabel, LV_TEXT_ALIGN_CENTER, 0);
    /*-----------------------创建Emoji标签--------------------------*/
    emojiLable = lv_label_create(scr);
    // 设置图片字体
    lv_obj_set_style_text_font(emojiLable, font_emoji_32_init(), 0);
    // 设置默认显示文字
    lv_label_set_text(emojiLable, "🙂");
    // 显示对齐方式[居中对齐, 往上偏移100个像素]
    lv_obj_align(emojiLable, LV_ALIGN_CENTER, 0, -100);
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