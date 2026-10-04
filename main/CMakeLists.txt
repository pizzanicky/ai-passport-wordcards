idf_component_register(
 SRCS "wc_app.c" "wc_model.c" "wc_store.c" "wc_ui.c" "wc_audio.c" "wc_decoder.c" "wc_data.c"
      "../assets/fonts/wc_font_16.c" "../assets/fonts/wc_font_18.c" "../assets/fonts/wc_font_20.c"
      "../assets/fonts/wc_font_22.c" "../assets/fonts/wc_font_24.c" "../assets/fonts/wc_font_26.c" "../assets/fonts/wc_font_30.c"
      "../assets/images/wc_portraits.c"
 INCLUDE_DIRS "."
 REQUIRES bsp nvs_flash esp_timer app_update esp_audio_codec esp_partition mbedtls
)
