#include "wc_audio.h"
#include "wc_feedback.h"
#include "wc_decoder.h"
#include "esp_log.h"
#include "wc_data.h"
#include "wc_audio_asset.h"
#include "esp_partition.h"
#include "mbedtls/sha256.h"
#include <string.h>
#include "bsp_audio.h"
#include "esp_audio_simple_dec.h"
#include "esp_audio_simple_dec_default.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include <stdatomic.h>
#include <stdlib.h>
static const esp_partition_t *audio_partition,*audio_first;
#define AUDIO_SPLIT 0x156000u
static esp_err_t audio_read(size_t offset,void *dst,size_t n){
 if(offset<AUDIO_SPLIT){size_t first=AUDIO_SPLIT-offset;if(first>n)first=n;esp_err_t e=esp_partition_read(audio_first,offset,dst,first);if(e!=ESP_OK)return e;offset+=first;dst=(uint8_t *)dst+first;n-=first;}
 return n?esp_partition_read(audio_partition,offset-AUDIO_SPLIT,dst,n):ESP_OK;
}
typedef struct {uint16_t word;uint8_t volume,kind;unsigned generation;} request_t;
static QueueHandle_t requests;
static SemaphoreHandle_t owner;
static atomic_uint generation;
static atomic_int last_error;
static esp_err_t prepare(uint8_t volume){
 esp_err_t e=bsp_audio_init();if(e!=ESP_OK)return e;e=bsp_audio_wake();if(e!=ESP_OK)return e;e=bsp_audio_set_format(16000,16,1);if(e!=ESP_OK)return e;bsp_audio_set_volume(volume);return ESP_OK;
}
static esp_err_t feedback(const request_t *r){
 const wc_feedback_t *sound=r->kind==1?&wc_feedback_known:&wc_feedback_again;
 esp_err_t e=prepare(r->volume);if(e!=ESP_OK)return e;
 for(uint32_t at=0;at<sound->count && atomic_load(&generation)==r->generation;at+=256){uint32_t n=sound->count-at;if(n>256)n=256;e=bsp_audio_write(sound->pcm+at,n*sizeof(int16_t));if(e!=ESP_OK)return e;}
 return ESP_OK;
}
static esp_err_t play(const request_t *r){
 const wc_word_t *w=&wc_words[r->word];size_t total=WC_AUDIO_BYTES;
 if(w->audio_offset>total || w->audio_length>total-w->audio_offset)return ESP_ERR_INVALID_SIZE;
 esp_err_t e=prepare(r->volume);if(e!=ESP_OK)return e;
 esp_audio_simple_dec_handle_t dec=NULL;esp_audio_simple_dec_cfg_t cfg={.dec_type=ESP_AUDIO_SIMPLE_DEC_TYPE_MP3};
 esp_audio_err_t open_result=esp_audio_simple_dec_open(&cfg,&dec);
 if(open_result!=ESP_AUDIO_ERR_OK){ESP_LOGE("word_audio","MP3 open failed: %d",open_result);return wc_decoder_error(open_result);}
 uint8_t *pcm=malloc(4096);if(!pcm){esp_audio_simple_dec_close(dec);return ESP_ERR_NO_MEM;}
 uint8_t *encoded=malloc(w->audio_length);if(!encoded){free(pcm);esp_audio_simple_dec_close(dec);return ESP_ERR_NO_MEM;}
 e=audio_read(w->audio_offset,encoded,w->audio_length);if(e!=ESP_OK){free(encoded);free(pcm);esp_audio_simple_dec_close(dec);return e;}
 const uint8_t *p=encoded;unsigned left=w->audio_length;bool played=false;unsigned decoded=0;
 while(left && atomic_load(&generation)==r->generation){
  esp_audio_simple_dec_raw_t raw={.buffer=(uint8_t *)p,.len=left,.eos=true};esp_audio_simple_dec_out_t out={.buffer=pcm,.len=4096};
  esp_audio_err_t rc=esp_audio_simple_dec_process(dec,&raw,&out);
  if(rc!=ESP_AUDIO_ERR_OK || raw.consumed>left || (!raw.consumed&&!out.decoded_size)){e=ESP_FAIL;break;}
  p+=raw.consumed;left-=raw.consumed;
  if(out.decoded_size){esp_audio_simple_dec_info_t info;
   if(esp_audio_simple_dec_get_info(dec,&info)!=ESP_AUDIO_ERR_OK || info.sample_rate!=16000 || info.channel!=1 || info.bits_per_sample!=16){e=ESP_ERR_INVALID_ARG;break;}
   e=bsp_audio_write(pcm,out.decoded_size);if(e!=ESP_OK)break;played=true;decoded+=out.decoded_size;
  }
 }
 if(!played && atomic_load(&generation)==r->generation && e==ESP_OK)e=ESP_FAIL;
 if(e==ESP_OK&&played)ESP_LOGI("word_audio","PCM bytes=%u stack remaining=%u",decoded,(unsigned)uxTaskGetStackHighWaterMark(NULL));
 free(encoded);free(pcm);esp_audio_simple_dec_close(dec);return e;
}
static void audio_worker(void *arg){(void)arg;request_t r;for(;;){if(xQueueReceive(requests,&r,portMAX_DELAY)!=pdTRUE)continue;xSemaphoreTake(owner,portMAX_DELAY);if(r.generation==atomic_load(&generation)){esp_err_t e=r.kind?feedback(&r):play(&r);if(e!=ESP_OK)atomic_store(&last_error,e);bsp_audio_sleep();}xSemaphoreGive(owner);}}
esp_err_t wc_audio_init(void){
 audio_partition=esp_partition_find_first(ESP_PARTITION_TYPE_DATA,0x40,"wordaudio");
 audio_first=esp_partition_find_first(ESP_PARTITION_TYPE_DATA,0x41,"audioa");
 if(!audio_partition||!audio_first||audio_first->address!=0x200000||audio_first->size!=AUDIO_SPLIT||audio_partition->address!=0x35a000||audio_partition->size+AUDIO_SPLIT<WC_AUDIO_BYTES)return ESP_ERR_NOT_FOUND;
 uint8_t *buffer=malloc(4096);if(!buffer)return ESP_ERR_NO_MEM;
 mbedtls_sha256_context sha;mbedtls_sha256_init(&sha);int rc=mbedtls_sha256_starts(&sha,0);esp_err_t result=ESP_OK;
 for(size_t at=0;at<WC_AUDIO_BYTES&&rc==0;at+=4096){size_t n=WC_AUDIO_BYTES-at;if(n>4096)n=4096;result=audio_read(at,buffer,n);if(result!=ESP_OK)break;rc=mbedtls_sha256_update(&sha,buffer,n);}
 uint8_t digest[32];if(rc==0&&result==ESP_OK)rc=mbedtls_sha256_finish(&sha,digest);mbedtls_sha256_free(&sha);free(buffer);
 if(result!=ESP_OK)return result;
 if(rc!=0||memcmp(digest,wc_audio_sha256,32))return ESP_ERR_INVALID_CRC;
 result=wc_decoder_setup();if(result!=ESP_OK)return result;
 ESP_LOGI("word_audio","MP3 decoder registered and supported");
 owner=xSemaphoreCreateMutex();requests=xQueueCreate(1,sizeof(request_t));
 if(!owner||!requests)return ESP_ERR_NO_MEM;
 if(xTaskCreate(audio_worker,"word_audio",8192,NULL,5,NULL)!=pdPASS)return ESP_ERR_NO_MEM;
 return ESP_OK;
}
bool wc_audio_play(uint16_t word,uint8_t volume){if(!requests||word>=WC_WORD_COUNT)return false;request_t r={.word=word,.volume=volume,.generation=atomic_fetch_add(&generation,1)+1};return xQueueOverwrite(requests,&r)==pdPASS;}
bool wc_audio_feedback(bool familiar,uint8_t volume){if(!requests)return false;request_t r={.kind=familiar?1:2,.volume=volume,.generation=atomic_fetch_add(&generation,1)+1};return xQueueOverwrite(requests,&r)==pdPASS;}
bool wc_audio_stop(void){if(!owner)return true;atomic_fetch_add(&generation,1);xQueueReset(requests);if(xSemaphoreTake(owner,pdMS_TO_TICKS(1000))!=pdTRUE)return false;xSemaphoreGive(owner);return true;}
esp_err_t wc_audio_take_error(void){return atomic_exchange(&last_error,ESP_OK);}
