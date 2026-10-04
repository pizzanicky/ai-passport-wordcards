#include "wc_model.h"
#include <assert.h>
#include <stdio.h>
static wc_state_t s,old;
int main(void){uint32_t ids[50];for(int i=0;i<50;i++)ids[i]=i+1;
 wc_init(&s,ids,50,123);assert(wc_valid(&s));assert(s.volume==100 && s.reserved[0]==1);assert(wc_start(&s));assert(s.length==5);assert(s.round==1);
 assert(wc_rate(&s,true));assert(s.cursor==1 && s.round==1 && s.words[0].due==2);
 uint16_t queue=s.queue[1];assert(wc_start(&s));assert(s.cursor==1 && s.queue[1]==queue);
 for(int i=1;i<5;i++)assert(wc_rate(&s,false));assert(s.round==2);assert(wc_due_count(&s)==5);
 assert(wc_start(&s));assert(s.length==10);assert(s.queue[0]==0);
 while(s.cursor<s.length)assert(wc_rate(&s,true));assert(s.round==3);
 for(int k=0;k<30;k++){assert(wc_start(&s));assert(s.length<=20);unsigned fresh=0;for(int j=0;j<s.length;j++)fresh+=!s.words[s.queue[j]].seen;assert(fresh<=5);while(s.cursor<s.length)assert(wc_rate(&s,true));}
 assert(wc_seen_count(&s)==50);assert(wc_familiar_count(&s)==50);
 wc_start(&s);unsigned at=s.queue[0];wc_rate(&s,false);assert(s.words[at].stage==0 && s.words[at].due==s.round+1);
 old=s;wc_init(&s,ids,50,999);assert(wc_migrate(&s,&old));assert(s.length==0 && s.round==old.round && s.words[at].stage==0);
 s.length=2;s.queue[0]=0;s.queue[1]=0;assert(!wc_valid(&s));s=old;s.words[0].stage=6;assert(!wc_valid(&s));
 assert(wc_crc32("123456789",9)==0xcbf43926u);puts("wc_model: PASS");return 0;}
