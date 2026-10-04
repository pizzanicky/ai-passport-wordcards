#include "wc_model.h"
#include <string.h>
#include <limits.h>
void wc_init(wc_state_t *s,const uint32_t *ids,uint16_t n,uint32_t corpus) {
 memset(s,0,sizeof(*s));s->round=1;s->volume=100;s->reserved[0]=1;s->corpus=corpus;s->count=n<=WC_MAX_WORDS?n:0;
 for(unsigned i=0;i<s->count;i++)s->words[i].id=ids[i];
}
void wc_upgrade_defaults(wc_state_t *s){if(s->reserved[0]==0){s->volume=100;s->reserved[0]=1;}}
bool wc_valid(const wc_state_t *s) {
 if(!s->round || s->round>UINT32_MAX-32 || !s->count || s->count>WC_MAX_WORDS || s->length>20 || s->cursor>s->length || s->volume>100 || s->revealed>1 || s->recognized+s->again!=s->cursor)return false;
 for(unsigned i=0;i<s->count;i++)if(s->words[i].stage>5 || s->words[i].seen>1 || !s->words[i].id)return false;
 for(unsigned i=0;i<s->length;i++){
  if(s->queue[i]>=s->count)return false;
  for(unsigned j=0;j<i;j++)if(s->queue[i]==s->queue[j])return false;
 }
 return true;
}
static bool queued(const wc_state_t *s,unsigned index) {
 for(unsigned i=0;i<s->length;i++)if(s->queue[i]==index)return true;
 return false;
}
static int earliest(const wc_state_t *s,bool due_only) {
 int best=-1;
 for(unsigned i=0;i<s->count;i++) {
  const wc_progress_t *p=&s->words[i];
  if(!p->seen || queued(s,i) || (due_only && p->due>s->round))continue;
  if(best<0 || p->due<s->words[best].due)best=(int)i;
 }
 return best;
}
bool wc_start(wc_state_t *s) {
 if(!wc_valid(s))return false;
 if(s->length && s->cursor<s->length)return true;
 s->length=s->cursor=s->recognized=s->again=s->revealed=0;
 int index;
 while(s->length<20 && (index=earliest(s,true))>=0)s->queue[s->length++]=(uint16_t)index;
 unsigned added=0;
 for(unsigned i=0;i<s->count && s->length<20 && added<5;i++)if(!s->words[i].seen){s->queue[s->length++]=(uint16_t)i;added++;}
 if(!s->length)while(s->length<20 && (index=earliest(s,false))>=0)s->queue[s->length++]=(uint16_t)index;
 return s->length>0;
}
bool wc_rate(wc_state_t *s,bool familiar) {
 if(!wc_valid(s) || s->cursor>=s->length || s->round>=UINT32_MAX-32)return false;
 wc_progress_t *p=&s->words[s->queue[s->cursor]];p->seen=1;
 if(familiar){if(p->stage<5)p->stage++;p->due=s->round+(1u<<(p->stage-1));s->recognized++;}
 else {p->stage=0;p->due=s->round+1;s->again++;}
 s->revealed=0;s->cursor++;
 if(s->cursor==s->length)s->round++;
 return true;
}
uint16_t wc_due_count(const wc_state_t *s){uint16_t n=0;for(unsigned i=0;i<s->count;i++)n+=s->words[i].seen&&s->words[i].due<=s->round;return n;}
uint16_t wc_seen_count(const wc_state_t *s){uint16_t n=0;for(unsigned i=0;i<s->count;i++)n+=s->words[i].seen!=0;return n;}
uint16_t wc_familiar_count(const wc_state_t *s){uint16_t n=0;for(unsigned i=0;i<s->count;i++)n+=s->words[i].stage>=3;return n;}
uint32_t wc_crc32(const void *data,unsigned n){const uint8_t *p=data;uint32_t c=~0u;while(n--){c^=*p++;for(unsigned j=0;j<8;j++)c=(c>>1)^(0xedb88320u&-(c&1));}return ~c;}
bool wc_migrate(wc_state_t *s,const wc_state_t *old) {
 if(!wc_valid(old)||!wc_valid(s))return false;
 unsigned found=0;s->round=old->round;s->volume=old->volume;s->reserved[0]=old->reserved[0];wc_upgrade_defaults(s);
 for(unsigned i=0;i<s->count;i++)for(unsigned j=0;j<old->count;j++)if(s->words[i].id==old->words[j].id){s->words[i]=old->words[j];found++;break;}
 if(old->corpus==s->corpus && old->count==s->count){
  memcpy(s->queue,old->queue,sizeof(s->queue));s->length=old->length;s->cursor=old->cursor;s->recognized=old->recognized;s->again=old->again;s->revealed=old->revealed;
 }
 return found>0;
}
