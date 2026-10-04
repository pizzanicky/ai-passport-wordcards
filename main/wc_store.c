#include "wc_store.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <stdlib.h>
#include <string.h>
#define MAGIC 0x57434331u
typedef struct {uint32_t magic,version,sequence,crc;wc_state_t data;} snapshot_t;
static nvs_handle_t handle;
static uint32_t sequence;
static int current_slot=-1;
static bool opened;
esp_err_t wc_store_open(void){esp_err_t e=nvs_flash_init();if(e!=ESP_OK)return e;e=nvs_open("wordcards",NVS_READWRITE,&handle);opened=e==ESP_OK;return e;}
static bool read_slot(int slot,snapshot_t *b,bool *present){
 size_t n=sizeof(*b);esp_err_t e=nvs_get_blob(handle,slot?"slot1":"slot0",b,&n);
 if(e==ESP_ERR_NVS_NOT_FOUND){*present=false;return false;}*present=true;
 if(e!=ESP_OK||n!=sizeof(*b))return false;
 uint32_t crc=b->crc;b->crc=0;bool valid=b->magic==MAGIC && b->version==1 && wc_crc32(b,sizeof(*b))==crc && wc_valid(&b->data);b->crc=crc;return valid;
}
esp_err_t wc_store_load(wc_state_t *s,bool *migrated){
 *migrated=false;if(!opened)return ESP_ERR_INVALID_STATE;
 snapshot_t *b=malloc(sizeof(*b));if(!b)return ESP_ERR_NO_MEM;
 bool any=false,present=false,had_data=false;uint32_t best=0;int slot=-1;
 for(int i=0;i<2;i++){bool valid=read_slot(i,b,&present);had_data|=present;if(valid && (!any || (int32_t)(b->sequence-best)>0)){best=b->sequence;slot=i;any=true;}}
 if(!any){free(b);return had_data?ESP_ERR_INVALID_CRC:ESP_ERR_NOT_FOUND;}
 if(!read_slot(slot,b,&present)){free(b);return ESP_FAIL;}
 bool ok;
 if(s->corpus==b->data.corpus && s->count==b->data.count){
  ok=true;for(unsigned i=0;i<s->count;i++)if(s->words[i].id!=b->data.words[i].id)ok=false;
  if(ok)*s=b->data;
 }else {ok=wc_migrate(s,&b->data);*migrated=ok;}
 if(ok){wc_upgrade_defaults(s);sequence=best;current_slot=slot;}
 free(b);return ok?ESP_OK:ESP_ERR_INVALID_VERSION;
}
esp_err_t wc_store_save(const wc_state_t *s){
 if(!opened || !wc_valid(s))return ESP_ERR_INVALID_STATE;
 snapshot_t *b=malloc(sizeof(*b));if(!b)return ESP_ERR_NO_MEM;
 b->magic=MAGIC;b->version=1;b->sequence=sequence+1;b->data=*s;b->crc=0;b->crc=wc_crc32(b,sizeof(*b));
 int slot=current_slot==0?1:0;esp_err_t e=nvs_set_blob(handle,slot?"slot1":"slot0",b,sizeof(*b));free(b);
 if(e==ESP_OK)e=nvs_commit(handle);
 if(e==ESP_OK){sequence++;current_slot=slot;}
 return e;
}
