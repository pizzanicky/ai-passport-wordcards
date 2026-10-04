#include "wc_store.h"
#include "nvs.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static unsigned char slots[2][60000],pending[60000];static size_t sizes[2],pending_size;static int pending_slot,fail_set,fail_commit;
esp_err_t nvs_flash_init(void){return ESP_OK;}
esp_err_t nvs_open(const char *name,int mode,nvs_handle_t *h){(void)name;(void)mode;*h=1;return ESP_OK;}
static int slot(const char *key){return !strcmp(key,"slot1");}
esp_err_t nvs_get_blob(nvs_handle_t h,const char *key,void *b,size_t *n){(void)h;int i=slot(key);if(!sizes[i])return ESP_ERR_NVS_NOT_FOUND;assert(*n>=sizes[i]);memcpy(b,slots[i],sizes[i]);*n=sizes[i];return ESP_OK;}
esp_err_t nvs_set_blob(nvs_handle_t h,const char *key,const void *b,size_t n){(void)h;if(fail_set)return ESP_FAIL;assert(n<=sizeof(pending));memcpy(pending,b,n);pending_size=n;pending_slot=slot(key);return ESP_OK;}
esp_err_t nvs_commit(nvs_handle_t h){(void)h;if(fail_commit)return ESP_FAIL;memcpy(slots[pending_slot],pending,pending_size);sizes[pending_slot]=pending_size;return ESP_OK;}
static wc_state_t a,b;
static void reboot(const uint32_t *ids,unsigned n,unsigned corpus){wc_init(&b,ids,n,corpus);assert(wc_store_open()==ESP_OK);}
int main(void){uint32_t ids[]={10,20,30};bool migrated;wc_init(&a,ids,3,77);assert(wc_store_open()==ESP_OK);assert(wc_store_load(&a,&migrated)==ESP_ERR_NOT_FOUND);
 assert(wc_start(&a));assert(wc_store_save(&a)==ESP_OK);wc_rate(&a,true);assert(wc_store_save(&a)==ESP_OK);reboot(ids,3,77);assert(wc_store_load(&b,&migrated)==ESP_OK && b.cursor==1);
 // Simulate power loss before commit: last acknowledged state remains intact.
 wc_rate(&a,false);fail_commit=1;assert(wc_store_save(&a)==ESP_FAIL);fail_commit=0;reboot(ids,3,77);assert(wc_store_load(&b,&migrated)==ESP_OK && b.cursor==1);
 // Corrupt the latest slot; previous valid snapshot is recovered.
 slots[1][100]^=0xff;reboot(ids,3,77);assert(wc_store_load(&b,&migrated)==ESP_OK && b.cursor==0);
 // Header corruption also invalidates the snapshot, including sequence bytes.
 slots[0][8]^=1;reboot(ids,3,77);assert(wc_store_load(&b,&migrated)==ESP_ERR_INVALID_CRC);
 memset(sizes,0,sizeof(sizes));wc_init(&a,ids,3,77);wc_store_open();wc_start(&a);wc_rate(&a,true);assert(wc_store_save(&a)==ESP_OK);
 uint32_t next[]={30,10,40};reboot(next,3,88);assert(wc_store_load(&b,&migrated)==ESP_OK && migrated && b.words[1].seen && b.length==0);
 // Upgrade the old default once, retaining the learning state and later user settings.
 wc_init(&a,ids,3,77);wc_start(&a);wc_rate(&a,true);a.volume=30;a.reserved[0]=0;assert(wc_store_save(&a)==ESP_OK);
 reboot(ids,3,77);assert(wc_store_load(&b,&migrated)==ESP_OK && b.volume==100 && b.reserved[0]==1 && b.cursor==1 && b.words[0].seen);
 b.volume=65;assert(wc_store_save(&b)==ESP_OK);reboot(ids,3,77);assert(wc_store_load(&b,&migrated)==ESP_OK && b.volume==65);
 // Relearning choice survives reboot; failed completion keeps the pending card.
 assert(wc_begin_relearn(&b));assert(wc_store_save(&b)==ESP_OK);
 reboot(ids,3,77);assert(wc_store_load(&b,&migrated)==ESP_OK&&b.revealed&&b.cursor==1);
 assert(wc_rate(&b,false));fail_commit=1;assert(wc_store_save(&b)==ESP_FAIL);fail_commit=0;
 reboot(ids,3,77);assert(wc_store_load(&b,&migrated)==ESP_OK&&b.revealed&&b.cursor==1);
 assert(wc_rate(&b,false));assert(wc_store_save(&b)==ESP_OK);
 reboot(ids,3,77);assert(wc_store_load(&b,&migrated)==ESP_OK&&!b.revealed&&b.cursor==2&&b.again==1);
 fail_set=1;assert(wc_store_save(&b)==ESP_FAIL);puts("wc_store: PASS");}
