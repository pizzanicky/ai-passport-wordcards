#include "wc_decoder.h"
#include "esp_audio_dec.h"
#include <assert.h>
#include <stdio.h>
static int calls,registered;
static esp_audio_err_t lower_result,support_result,simple_result;
esp_audio_err_t esp_audio_dec_register_default(void){calls++;if(lower_result==ESP_AUDIO_ERR_OK)registered=1;return lower_result;}
esp_audio_err_t esp_audio_dec_check_audio_type(esp_audio_type_t type){assert(type==ESP_AUDIO_TYPE_MP3);calls++;return registered?support_result:ESP_AUDIO_ERR_NOT_SUPPORT;}
esp_audio_err_t esp_audio_simple_dec_register_default(void){calls++;assert(registered);return simple_result;}
static void reset(void){calls=registered=0;lower_result=support_result=simple_result=ESP_AUDIO_ERR_OK;}
int main(void){
 reset();assert(wc_decoder_setup()==ESP_OK && calls==3 && registered);
 reset();lower_result=ESP_AUDIO_ERR_MEM_LACK;assert(wc_decoder_setup()==ESP_ERR_NO_MEM && calls==1);
 reset();support_result=ESP_AUDIO_ERR_NOT_SUPPORT;assert(wc_decoder_setup()==ESP_ERR_NOT_SUPPORTED && calls==2);
 reset();simple_result=ESP_AUDIO_ERR_INVALID_PARAMETER;assert(wc_decoder_setup()==ESP_ERR_INVALID_ARG && calls==3);
 assert(wc_decoder_error(ESP_AUDIO_ERR_NOT_SUPPORT)!=ESP_ERR_NO_MEM);
 assert(wc_decoder_error(ESP_AUDIO_ERR_HEADER_PARSE)==ESP_FAIL);
 puts("Decoder registration/error regression: PASS");
}
