#include "wc_decoder.h"
#include "esp_audio_dec.h"
#include "esp_audio_dec_default.h"
#include "esp_audio_simple_dec_default.h"
esp_err_t wc_decoder_error(esp_audio_err_t result){
 if(result==ESP_AUDIO_ERR_OK)return ESP_OK;
 if(result==ESP_AUDIO_ERR_MEM_LACK)return ESP_ERR_NO_MEM;
 if(result==ESP_AUDIO_ERR_NOT_SUPPORT)return ESP_ERR_NOT_SUPPORTED;
 if(result==ESP_AUDIO_ERR_INVALID_PARAMETER)return ESP_ERR_INVALID_ARG;
 return ESP_FAIL;
}
esp_err_t wc_decoder_setup(void){
 esp_audio_err_t result=esp_audio_dec_register_default();
 if(result!=ESP_AUDIO_ERR_OK)return wc_decoder_error(result);
 result=esp_audio_dec_check_audio_type(ESP_AUDIO_TYPE_MP3);
 if(result!=ESP_AUDIO_ERR_OK)return wc_decoder_error(result);
 return wc_decoder_error(esp_audio_simple_dec_register_default());
}
