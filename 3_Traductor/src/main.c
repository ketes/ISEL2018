#include "esp_common.h"
#include "freertos/task.h"
#include "gpio.h"

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;
    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

void task_blink(void* ignore)
{
    gpio16_output_conf();
    while(true) {
    	gpio16_output_set(0);
        vTaskDelay(1000/portTICK_RATE_MS);
    	gpio16_output_set(1);
        vTaskDelay(1000/portTICK_RATE_MS);
    }

    vTaskDelete(NULL);
}
/* Devuelve la correspondencia en Morse del carácter c */
const char* morse(char c){
  static const char* morse_ch[] = {
    ".-",   /* A */
    "-...", /* B */
    "-.-.", /* C */
    "-..", /* D */
    ".", /* E */
    "..-.", /* F */
    "--.", /* G */
    "....", /* H */
    "..", /* I */
    ".---", /* J */
    "-.-", /* K */
    ".-..", /* L */
    "--", /* M */
    "-.", /* N */
    "---", /* O */
    ".--.", /* P */
    "--.-", /* Q */
    ".-.", /* R */
    "...", /* S */
    "-", /* T */
    "..-", /* U */
    "...-", /* V */
    ".--", /* W */
    "-..-", /* X */
    "-.--", /* Y */
    "--..", /* Z */
    "  ",
    "    "
  };
  return morse_ch[c - 'a'];
}
/* Copia en buf la versión en Morse del mensaje str, con un límite de n caractes */
int str2morse ( char *buf , int n ,const char* str ) {
  int i;
  char dir;
  int d_aux;
  d_aux = 0;
  for (i = 0; i < n && str[i] != '\0'; i++) {
    dir = str[i];
    if ( dir == ' ') {
      buf[d_aux]=*morse('1');
      d_aux =  d_aux + sizeof(morse('1'));
    }
    else    {
      buf[d_aux]=*morse(dir);
      d_aux += sizeof(morse(dir));
      buf[d_aux]=*morse('0');
      d_aux += sizeof(morse('0'));
    }
  };
  return 0;
}
/* Envía el mensaje msg, ya codificado en Morse, encendiendo y apagando el LED */
void morse_send (const char* msg){
  switch(*msg){
    case '.':
      GPIO_OUTPUT_SET(2, 1);
      vTaskDelay(250/portTICK_RATE_MS);
      GPIO_OUTPUT_SET(2, 0);
      vTaskDelay(250/portTICK_RATE_MS);
    case '-':
      GPIO_OUTPUT_SET(2, 1);
      vTaskDelay(750/portTICK_RATE_MS);
      GPIO_OUTPUT_SET(2, 0);
      vTaskDelay(250/portTICK_RATE_MS);
    case ' ':
      GPIO_OUTPUT_SET(2, 0);
      vTaskDelay(250/portTICK_RATE_MS);
    case '\0':
      return;
  }
  morse_send(++msg);
}


/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
char *buf;
const char *str = "hola mundo";;
int ret;
ret =  str2morse ( buf , 10 , str );
xTaskCreate(&morse_send , "startup", 2048, buf, 1, NULL);
}
