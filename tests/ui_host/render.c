#include "lvgl.h"
#include "wc_ui.h"
#include "wc_data.h"
#include "bsp_display_rounding.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
static uint16_t fb[240*320],buffer[240*20];static wc_state_t state;
static void flush(lv_display_t *display,const lv_area_t *a,uint8_t *data){uint16_t *p=(uint16_t *)data;for(int y=a->y1;y<=a->y2;y++)for(int x=a->x1;x<=a->x2;x++){uint16_t value=*p++;fb[y*240+x]=bsp_display_pixel_outside_rounded_rect(x,y,240,320,30)?0:value;}lv_display_flush_ready(display);}
static void write_frame(const char *dir,const char *name){char path[512];snprintf(path,sizeof(path),"%s/%s.ppm",dir,name);FILE *f=fopen(path,"wb");assert(f);fprintf(f,"P6\n240 320\n255\n");for(int i=0;i<240*320;i++){unsigned v=fb[i];unsigned char p[3]={(v>>11)*255/31,((v>>5)&63)*255/63,(v&31)*255/31};fwrite(p,1,3,f);}fclose(f);}
static void check_labels(lv_obj_t *obj){
 if(lv_obj_check_type(obj,&lv_label_class)){
  const unsigned char *p=(const unsigned char *)lv_label_get_text(obj);const lv_font_t *font=lv_obj_get_style_text_font(obj,0);
  while(*p){uint32_t c=*p++;if(c>=0xc0){unsigned n=c<0xe0?1:c<0xf0?2:3;c&=(1u<<(6-n))-1;while(n--){assert((*p&0xc0)==0x80);c=(c<<6)|(*p++&0x3f);}}
   if(c=='\n'||c=='\r')continue;lv_font_glyph_dsc_t glyph;if(!lv_font_get_glyph_dsc(font,&glyph,c,0) || glyph.is_placeholder){fprintf(stderr,"Missing U+%04x, line_height=%d, label=%s\n",c,font->line_height,lv_label_get_text(obj));abort();}
  }
 }
 for(unsigned i=0;i<lv_obj_get_child_count(obj);i++)check_labels(lv_obj_get_child(obj,i));
}
int main(int argc,char **argv){assert(LV_USE_FONT_COMPRESSED==1);assert(argc==2);lv_init();lv_display_t *display=lv_display_create(240,320);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_buffers(display,buffer,NULL,sizeof(buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 wc_init(&state,wc_word_ids,WC_WORD_COUNT,WC_CORPUS_ID);wc_start(&state);state.round=12;unsigned promise=0,longword=0;for(unsigned i=0;i<WC_WORD_COUNT;i++){if(!strcmp(wc_words[i].word,"promise"))promise=i;if(!strcmp(wc_words[i].word,"responsibility"))longword=i;}
 state.queue[0]=promise;wc_ui_init();wc_view_t view={.page=WC_HOME,.battery=86};const char *names[]={"home","front","back","reading","finish","progress","settings","volume","reset","about","error"};
 for(int p=0;p<=WC_ERROR;p++){view.page=p;view.error="进度未保存，请重试";view.read_page=0;view.read_pages=wc_ui_definition_pages(state.queue[0]);wc_ui_render(&state,&view);lv_refr_now(display);check_labels(lv_screen_active());write_frame(argv[1],names[p]);}
 state.queue[0]=longword;view.page=WC_READ;view.read_pages=wc_ui_definition_pages(longword);wc_ui_render(&state,&view);lv_refr_now(display);check_labels(lv_screen_active());write_frame(argv[1],"long-word");
 unsigned longest=0;for(unsigned i=1;i<WC_WORD_COUNT;i++)if(strlen(wc_words[i].definition)>strlen(wc_words[longest].definition))longest=i;state.queue[0]=longest;view.page=WC_READ;view.read_pages=wc_ui_definition_pages(longest);for(unsigned i=0;i<view.read_pages;i++){view.read_page=i;wc_ui_render(&state,&view);lv_refr_now(display);check_labels(lv_screen_active());char name[64];snprintf(name,sizeof(name),"long-definition-%u",i+1);write_frame(argv[1],name);}
 // Check every actual corpus entry against layout/page limits, plus UI memory stability.
 for(unsigned i=0;i<WC_WORD_COUNT;i++){uint8_t pages=wc_ui_definition_pages(i);assert(pages>0&&pages<30);state.queue[0]=i;view.page=WC_READ;view.read_page=0;view.read_pages=pages;wc_ui_render(&state,&view);check_labels(lv_screen_active());}
 for(unsigned i=0;i<100;i++){view.page=i%11;wc_ui_render(&state,&view);lv_refr_now(display);}
 lv_mem_monitor_t m;lv_mem_monitor(&m);printf("UI host: PASS, pool used=%u%% largest=%zu free=%zu\n",m.used_pct,m.free_biggest_size,m.free_size);return 0;}
