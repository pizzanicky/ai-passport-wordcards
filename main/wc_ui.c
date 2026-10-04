#include "wc_ui.h"
#include "wc_data.h"
#include "lvgl.h"
#include "bsp_display.h"
#include "esp_app_desc.h"
#include <stdio.h>
#include <string.h>
LV_FONT_DECLARE(wc_font_16);LV_FONT_DECLARE(wc_font_18);LV_FONT_DECLARE(wc_font_20);LV_FONT_DECLARE(wc_font_24);LV_FONT_DECLARE(wc_font_26);LV_FONT_DECLARE(wc_font_30);LV_FONT_DECLARE(wc_font_22);
LV_IMAGE_DECLARE(wc_portrait_home);LV_IMAGE_DECLARE(wc_portrait_front);LV_IMAGE_DECLARE(wc_portrait_finish);
#define PAPER 0xFBF5EB
#define INK 0x343654
#define WINE 0x925052
#define TEAL 0x638D98
#define DARK_TEAL 0x3E6570
#define SLATE 0x899BAE
#define LIGHT 0xD5E6E3
#define PINK 0xE9D9DC
static lv_obj_t *screen;
static const lv_font_t *f(unsigned n){switch(n){case 16:return &wc_font_16;case 18:return &wc_font_18;case 24:return &wc_font_24;case 26:return &wc_font_26;case 30:return &wc_font_30;case 22:return &wc_font_22;default:return &wc_font_20;}}
static lv_obj_t *box(lv_obj_t *parent,int x,int y,int w,int h,uint32_t color,int radius){
 lv_obj_t *o=lv_obj_create(parent);lv_obj_remove_style_all(o);lv_obj_set_pos(o,x,y);lv_obj_set_size(o,w,h);lv_obj_set_style_bg_color(o,lv_color_hex(color),0);lv_obj_set_style_bg_opa(o,LV_OPA_COVER,0);lv_obj_set_style_radius(o,radius,0);lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE);return o;
}
static lv_obj_t *label_at(lv_obj_t *p,int x,int y,int width,const char *text,unsigned size,uint32_t color){
 lv_obj_t *o=lv_label_create(p);lv_obj_set_style_text_font(o,f(size),0);lv_obj_set_style_text_color(o,lv_color_hex(color),0);lv_obj_set_style_text_line_space(o,0,0);lv_obj_set_style_text_letter_space(o,0,0);lv_obj_set_pos(o,x,y);lv_obj_set_width(o,width);lv_label_set_text(o,text);return o;
}
static void text(int x,int y,int width,const char *t,unsigned size,uint32_t color){label_at(screen,x,y,width,t,size,color);}
static void divider(int y){box(screen,20,y,200,1,SLATE,0);}
static void portrait(int x,int y,const lv_image_dsc_t *src){lv_obj_t *o=lv_image_create(screen);lv_image_set_src(o,src);lv_obj_set_pos(o,x,y);}
static void footer(wc_page_t page){
 divider(249);
 if(page==WC_FRONT){text(20,259,90,"↑ 发音",16,INK);text(112,259,110,"OK 看释义",16,INK);}
 else if(page==WC_BACK){box(screen,17,257,89,27,LIGHT,4);box(screen,124,257,98,27,PINK,4);text(23,257,80,"↑ 认识",18,INK);text(133,257,80,"↓ 再练",18,INK);text(20,287,92,"OK 发音",16,INK);text(124,287,98,"长按OK返回",16,INK);return;}
 else {text(20,259,100,"↑↓ 翻页",16,INK);text(126,259,94,"OK 评价",16,INK);}
 text(43,287,175,"长按 OK 返回首页",16,INK);
}
static void title(const char *t){text(20,20,178,t,24,INK);}
static void main_action(int y,const char *t,bool selected){if(selected)box(screen,20,y,200,41,WINE,6);text(32,y+6,176,t,20,selected?PAPER:INK);}
static unsigned word_size(const char *word){lv_point_t sz;unsigned sizes[]={30,26,24,22};for(unsigned i=0;i<4;i++){lv_text_get_size(&sz,word,f(sizes[i]),0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);if(sz.x<=200)return sizes[i];}return 22;}
static void definition_geometry(uint16_t word,int *y,int *stride){
 const wc_word_t *w=&wc_words[word];lv_point_t word_size_px,ipa_size_px;
 lv_text_get_size(&word_size_px,w->word,f(word_size(w->word)),0,0,200,LV_TEXT_FLAG_NONE);
 lv_text_get_size(&ipa_size_px,w->ipa,&wc_font_16,0,0,200,LV_TEXT_FLAG_NONE);
 *y=78+word_size_px.y+6+ipa_size_px.y+10;
 int lines=(224-*y)/wc_font_20.line_height;if(lines<1)lines=1;
 *stride=wc_font_20.line_height*lines;
}
uint8_t wc_ui_definition_pages(uint16_t word){if(word>=WC_WORD_COUNT)return 1;lv_point_t sz;lv_text_get_size(&sz,wc_words[word].definition,&wc_font_20,0,0,200,LV_TEXT_FLAG_NONE);int y,stride;definition_geometry(word,&y,&stride);return (uint8_t)((sz.y+stride-1)/stride);}
void wc_ui_init(void){screen=lv_obj_create(NULL);lv_obj_remove_style_all(screen);lv_obj_set_style_bg_color(screen,lv_color_hex(PAPER),0);lv_obj_set_style_bg_opa(screen,LV_OPA_COVER,0);lv_obj_set_size(screen,240,320);lv_obj_remove_flag(screen,LV_OBJ_FLAG_SCROLLABLE);lv_screen_load(screen);}
void wc_ui_render(const wc_state_t *s,wc_view_t *v){
 lv_obj_clean(screen);char b[96];uint16_t index=s->cursor<s->length?s->queue[s->cursor]:0;const wc_word_t *w=&wc_words[index];
 if(v->page==WC_HOME){
  text(20,20,178,"随身词卡",26,INK);snprintf(b,sizeof(b),"第 %lu 轮",(unsigned long)s->round);text(20,65,115,b,18,DARK_TEAL);
  unsigned fresh=0;for(unsigned i=s->cursor;i<s->length;i++)fresh+=!s->words[s->queue[i]].seen;
  if(s->length && s->cursor<s->length)snprintf(b,sizeof(b),"%u 词",s->length-s->cursor);else {unsigned due=wc_due_count(s),unseen=s->count-wc_seen_count(s);fresh=unseen<5?unseen:5;if(due>=20)fresh=0;else if(fresh>20-due)fresh=20-due;snprintf(b,sizeof(b),"%u 词",due>=20?20:due+fresh<20?due+fresh:20);if(!due&&!fresh)snprintf(b,sizeof(b),"%u 词",s->count<20?s->count:20);}
  text(20,97,115,b,26,INK);snprintf(b,sizeof(b),"新词 %u",fresh);text(20,135,112,b,16,INK);portrait(132,56,&wc_portrait_home);
  main_action(182,s->length&&s->cursor<s->length?"› 继续学习":"› 开始学习",v->selection==0);
  if(v->selection==1){box(screen,20,230,200,27,LIGHT,4);}
  text(26,229,190,"学习进度",18,INK);
  if(v->selection==2){box(screen,20,256,200,27,LIGHT,4);}
  text(26,255,190,"设置",18,INK);
  divider(284);text(49,289,172,"↑↓ 选择  OK 进入",16,INK);
 }else if(v->page==WC_FRONT||v->page==WC_BACK||v->page==WC_READ){
  box(screen,20,18,58,25,WINE,5);text(32,18,42,s->words[index].seen?"复习":"新词",16,PAPER);
  snprintf(b,sizeof(b),"%02u / %02u",s->cursor+1,s->length);text(156,19,65,b,16,INK);divider(55);
  for(unsigned i=0;i<s->length;i++)box(screen,20+i*(200/s->length),64,7,3,i<s->cursor?TEAL:LIGHT,0);
  int wy=v->page==WC_FRONT?138:78;unsigned ws=word_size(w->word);lv_obj_t *wl=label_at(screen,20,wy,200,w->word,ws,INK);lv_obj_update_layout(wl);
  int ph=wy+lv_obj_get_height(wl)+6;text(20,ph,200,w->ipa,16,INK);
  if(v->page==WC_FRONT){text(20,90,152,"先回忆它的意思",16,DARK_TEAL);portrait(174,76,&wc_portrait_front);}
  else {int y,stride;definition_geometry(index,&y,&stride);lv_obj_t *clip=box(screen,20,y,200,stride,PAPER,0);label_at(clip,0,-v->read_page*stride,200,w->definition,20,INK);}
  if(v->page==WC_READ){snprintf(b,sizeof(b),"%u/%u",v->read_page+1,v->read_pages);text(183,226,37,b,16,DARK_TEAL);}
  footer(v->page);
 }else if(v->page==WC_FINISH){
  title("本轮完成");snprintf(b,sizeof(b),"第 %lu 轮",(unsigned long)(s->round-1));text(20,62,120,b,16,DARK_TEAL);portrait(153,72,&wc_portrait_finish);
  snprintf(b,sizeof(b),"认识 %u",s->recognized);text(20,98,130,b,22,INK);snprintf(b,sizeof(b),"再练 %u",s->again);text(20,133,130,b,22,INK);
  text(20,180,200,"再练的词，下轮再见。",18,INK);main_action(224,"OK 开始下一轮",true);text(43,289,175,"长按 OK 返回首页",16,INK);
 }else if(v->page==WC_PROGRESS){
  title("学习进度");const char *names[]={"已接触","熟悉","待复习","词库总数"};unsigned values[]={wc_seen_count(s),wc_familiar_count(s),wc_due_count(s),s->count};
  for(int i=0;i<4;i++){int y=i==3?224:73+i*43;text(20,y,120,names[i],18,INK);snprintf(b,sizeof(b),"%u",values[i]);text(150,y-2,70,b,24,INK);}divider(206);text(43,260,180,"依据你的自评记录",16,DARK_TEAL);text(43,289,175,"长按 OK 返回首页",16,INK);
 }else if(v->page==WC_SETTINGS){
  title("设置");const char *rows[]={"音量","重置学习记录","版本与词库"};for(int i=0;i<3;i++){int y=77+i*57;if(v->selection==i)box(screen,18,y,204,39,LIGHT,5);text(28,y+5,186,rows[i],20,INK);}
  snprintf(b,sizeof(b),"%u%%",s->volume);text(164,83,53,b,20,INK);text(20,240,200,"全部功能离线使用",16,DARK_TEAL);divider(282);text(49,289,172,"↑↓ 选择  OK 进入",16,INK);
 }else if(v->page==WC_VOLUME){title("声音音量");snprintf(b,sizeof(b),"%u%%",s->volume);text(70,108,130,b,30,INK);text(20,178,200,"↑ 增大  ↓ 减小",18,INK);text(20,222,200,"OK 保存并返回",18,INK);text(43,289,175,"长按 OK 返回首页",16,INK);
 }else if(v->page==WC_RESET){title("重置学习记录");text(20,83,200,"将清除全部学习进度。",18,INK);text(20,132,200,"词库和发音保持可用。",18,INK);main_action(198,"OK 确认清除",true);text(43,289,175,"长按 OK 取消返回",16,INK);
 }else if(v->page==WC_ABOUT){title("版本与词库");snprintf(b,sizeof(b),"版本 %s",esp_app_get_description()->version);text(20,80,200,b,16,INK);snprintf(b,sizeof(b),"高中词库 %u 词",s->count);text(20,121,200,b,18,INK);text(20,163,200,"英式发音 · 轮次复习",18,INK);text(20,207,200,"词库待发布审核",16,DARK_TEAL);text(43,289,175,"长按 OK 返回首页",16,INK);
 }else {title("需要处理");text(20,83,200,v->error?v->error:"操作未完成",18,INK);text(20,194,200,"OK 返回后重试",18,INK);text(43,289,175,"长按 OK 返回首页",16,INK);}
 // Approved artwork reserves the top-right motif area: keep battery on non-card pages only.
 if(v->page!=WC_FRONT&&v->page!=WC_BACK&&v->page!=WC_READ){if(v->battery>=0)snprintf(b,sizeof(b),"%d%%",v->battery);else strcpy(b,"--");text(191,20,33,b,16,DARK_TEAL);}
 if(v->busy)text(20,227,200,"正在保存…",16,DARK_TEAL);
}
