#include "wc_model.h"
#include "wc_data.h"
#include "wc_store.h"
#include "wc_audio.h"
#include "wc_ui.h"
#include "bsp_display.h"
#include "bsp_button.h"
#include "bsp_battery.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
static wc_state_t state;
static wc_view_t view={.page=WC_HOME,.battery=-1};
static QueueHandle_t inputs;
static bool store_ready,audio_ready,sleeping,progress_blocked;
static uint8_t original_volume;
static atomic_uint epoch;
typedef struct {bsp_btn_t btn;bsp_btn_ev_t event;unsigned epoch;int64_t at;} input_t;
static void button_cb(bsp_btn_t btn,bsp_btn_ev_t ev,void *user){(void)user;input_t i={.btn=btn,.event=ev,.epoch=atomic_load(&epoch),.at=esp_timer_get_time()};if(inputs)xQueueSend(inputs,&i,0);}
static void render(void){if(bsp_lvgl_lock(1000)){wc_ui_render(&state,&view);bsp_lvgl_unlock();}}
static void go(wc_page_t page){wc_audio_stop();view.page=page;view.read_page=0;atomic_fetch_add(&epoch,1);render();}
static void error(const char *message,wc_page_t back){view.error=message;view.return_page=back;go(WC_ERROR);}
static bool persist(void){
 if(!store_ready){error("存储不可用，进度未保存",view.page);return false;}
 if(!wc_audio_stop()){error("音频未停止，请重试",view.page);return false;}
 esp_err_t e=wc_store_save(&state);if(e!=ESP_OK){ESP_LOGE("wordcards","save: %s",esp_err_to_name(e));error("进度未保存，请重试",view.page);return false;}
 return true;
}
static bool start_round(void){
 if(progress_blocked){error("读取进度失败，请在设置确认重置",WC_HOME);return false;}
 unsigned char old[offsetof(wc_state_t,words)];memcpy(old,&state,sizeof(old));
 bool ok=wc_start(&state)&&persist();if(!ok)memcpy(&state,old,sizeof(old));
 if(ok){if(state.revealed && bsp_lvgl_lock(1000)){view.read_pages=wc_ui_definition_pages(state.queue[state.cursor]);bsp_lvgl_unlock();}go(state.revealed?(view.read_pages>1?WC_READ:WC_BACK):WC_FRONT);}return ok;
}
static void rate(bool familiar){
 unsigned at=state.queue[state.cursor];wc_progress_t old=state.words[at];uint8_t cursor=state.cursor,recognized=state.recognized,again=state.again,revealed=state.revealed;uint32_t round=state.round;
 if(!wc_rate(&state,familiar))return;
 if(!persist()){state.words[at]=old;state.cursor=cursor;state.recognized=recognized;state.again=again;state.revealed=revealed;state.round=round;return;}
 go(state.cursor==state.length?WC_FINISH:WC_FRONT);
}
static void reveal(void){
 state.revealed=1;if(!persist()){state.revealed=0;return;}
 if(bsp_lvgl_lock(1000)){view.read_pages=wc_ui_definition_pages(state.queue[state.cursor]);bsp_lvgl_unlock();}
 go(view.read_pages>1?WC_READ:WC_BACK);
}
static void play_word(void){if(!audio_ready || !wc_audio_play(state.queue[state.cursor],state.volume))error("发音资源或设备不可用",view.page);}
static void dispatch(bsp_btn_t b,bsp_btn_ev_t ev){
 if(ev==BSP_BTN_LONG && b==BSP_BTN_OK){if(view.page==WC_VOLUME||(view.page==WC_ERROR&&view.return_page==WC_VOLUME))state.volume=original_volume;view.selection=0;go(WC_HOME);return;}
 if(ev!=BSP_BTN_CLICK && ev!=BSP_BTN_DOUBLE)return;
 bool up=b==BSP_BTN_UP,down=b==BSP_BTN_DOWN,ok=b==BSP_BTN_OK;
 switch(view.page){
 case WC_HOME:
 case WC_SETTINGS:{unsigned max=2;if(up&&view.selection)view.selection--;if(down&&view.selection<max)view.selection++;
  if(ok){if(view.page==WC_HOME){if(view.selection==0){start_round();return;}go(view.selection==1?WC_PROGRESS:WC_SETTINGS);view.selection=0;}else {original_volume=state.volume;go(view.selection==0?WC_VOLUME:view.selection==1?WC_RESET:WC_ABOUT);}}break;}
 case WC_FRONT:if(up)play_word();if(ok)reveal();return;
 case WC_BACK:if(up)rate(true);else if(down)rate(false);else if(ok)play_word();return;
 case WC_READ:if(up&&view.read_page)view.read_page--;if(down&&view.read_page+1<view.read_pages)view.read_page++;if(ok){go(WC_BACK);return;}break;
 case WC_FINISH:if(ok)start_round();return;
 case WC_VOLUME:{if(up&&state.volume<100)state.volume+=5;if(down&&state.volume>=5)state.volume-=5;if(ok){if(persist())go(WC_SETTINGS);else state.volume=original_volume;return;}break;}
 case WC_RESET:if(ok){wc_state_t *old=malloc(sizeof(state));if(!old){error("内部内存不足",WC_RESET);return;}*old=state;wc_init(&state,wc_word_ids,WC_WORD_COUNT,WC_CORPUS_ID);state.volume=old->volume;if(persist()){progress_blocked=false;view.selection=0;go(WC_HOME);}else state=*old;free(old);}return;
 case WC_ERROR:if(ok)go(view.return_page);return;
 default:break;
 }
 render();
}
static void app_worker(void *arg){
 (void)arg;int64_t last=esp_timer_get_time(),battery_at=last;bool wake_suppress[3]={0};input_t e;
 for(;;){
  if(xQueueReceive(inputs,&e,pdMS_TO_TICKS(100))==pdTRUE){
   if(sleeping){sleeping=false;last=esp_timer_get_time();bsp_display_backlight(80);wake_suppress[e.btn]=true;if(e.event==BSP_BTN_CLICK||e.event==BSP_BTN_DOUBLE)wake_suppress[e.btn]=false;continue;}
   last=esp_timer_get_time();if(wake_suppress[e.btn]){if(e.event!=BSP_BTN_PRESS)wake_suppress[e.btn]=false;continue;}
   if(e.epoch==atomic_load(&epoch))dispatch(e.btn,e.event);
  }
  int64_t now=esp_timer_get_time();
  if(!sleeping&&now-last>=60000000){wc_audio_stop();bsp_display_backlight(0);sleeping=true;}
  if(now-battery_at>=30000000){battery_at=now;view.battery=bsp_battery_soc();if(!sleeping)render();}
  esp_err_t audio_error=wc_audio_take_error();if(audio_error!=ESP_OK){ESP_LOGW("wordcards","audio: %s",esp_err_to_name(audio_error));if(!sleeping)error("发音未完成，请重试",view.page);}
 }
}
void app_main(void){
 wc_init(&state,wc_word_ids,WC_WORD_COUNT,WC_CORPUS_ID);
 esp_err_t e=wc_store_open();store_ready=e==ESP_OK;
 if(store_ready){bool migrated=false;e=wc_store_load(&state,&migrated);if(e==ESP_ERR_INVALID_VERSION){progress_blocked=true;view.error="词库迁移失败，请确认重置";}else if(e!=ESP_OK&&e!=ESP_ERR_NOT_FOUND){progress_blocked=true;view.error="读取进度失败，请在设置确认重置";}if(migrated)ESP_LOGI("wordcards","Progress migrated by stable word IDs");}
 if(bsp_display_init()!=ESP_OK||!bsp_lvgl_init()){ESP_LOGE("wordcards","Display unavailable");return;}
 bsp_battery_init();view.battery=bsp_battery_soc();audio_ready=wc_audio_init()==ESP_OK;
 inputs=xQueueCreate(16,sizeof(input_t));if(!inputs){ESP_LOGE("wordcards","Input queue unavailable");return;}
 if(bsp_lvgl_lock(1000)){wc_ui_init();wc_ui_render(&state,&view);bsp_lvgl_unlock();}
 if(progress_blocked)error(view.error,WC_HOME);
 if(bsp_button_init(button_cb,NULL)!=ESP_OK){error("按钮初始化失败",WC_HOME);return;}
 if(xTaskCreate(app_worker,"wordcards",6144,NULL,4,NULL)!=pdPASS){ESP_LOGE("wordcards","App worker unavailable");return;}
 bsp_display_backlight(80);
 ESP_LOGI("wordcards","words=%u free=%u largest=%u",state.count,(unsigned)esp_get_free_heap_size(),(unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
}
