
#include "Common_Config.h"
uint8_t device_id[18];
uint8_t client_id[37];

//websocket url
char* websocket_url;
//token
char* token;
//激活码
char* activation_code;


bool is_wakup = false;

// 全局事件标志组
EventGroupHandle_t global_event;

// 定义缓存数据的buffer 全局缓存的数组
RingbufHandle_t es8311_to_sr_buffer, sr_to_encoder_buff, encoder_to_ws_buff, ws_to_decoder_buff;



CommunicationStatus communicationStatus = IDLE;
char* session_id;
