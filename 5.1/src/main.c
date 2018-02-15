#include "esp_common.h"
#include "freertos/task.h"
#include "gpio.h"
#include "fsm.h"

/* Declaración de variables de sistema */

#define periodTick 100/portTICK_RATE_MS
#define tiempoGuarda 120/portTICK_RATE_MS
#define tiempoFin 60000/portTICK_RATE_MS
volatile int timeout0 = 0;
volatile int timeout1 = 0;
enum fsm_state {
  LED_ON,
  LED_OFF
};




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

/* Activacion con antirrebote */
int button_pressed(fsm_t*this) {
  if(!GPIO_INPUT_GET(0)||GPIO_INPUT_GET(15)){
    if (xTaskGetTickCount () < timeout0) {
  		timeout0 = xTaskGetTickCount () + tiempoGuarda;
      timeout1 = xTaskGetTickCount () + tiempoFin;
  		return 0;
  	}
  	timeout0 = xTaskGetTickCount () + tiempoGuarda ;
    timeout1 = xTaskGetTickCount () + tiempoFin;
    return 1;
  }
  return 0;


}
/* Desactivacion temporizada */
int fin_min (fsm_t*this){
  if(!GPIO_INPUT_GET(0)||GPIO_INPUT_GET(15)){
    if (xTaskGetTickCount () < timeout1) {
        timeout1 = xTaskGetTickCount () + tiempoFin;
      return 0;
    }
  }
  if(xTaskGetTickCount () >= timeout1){
    return 1;
  }
  return 0;
}
/* Activa el LED */
void led_a (fsm_t*this){
  GPIO_OUTPUT_SET(2, 0);
}
/* Desactiva el LED */
void led_d (fsm_t*this){
  GPIO_OUTPUT_SET(2, 1);
}

static fsm_trans_t mat_trans[]={
  {LED_OFF,button_pressed,LED_ON,led_a},
  {LED_ON,fin_min,LED_OFF,led_d},
  {-1,NULL,-1,NULL},
};

void inicio(void* ignore)
{

    fsm_t* fsm = fsm_new(mat_trans);
    led_d(fsm);
    portTickType xLastWakeTime;
    while(true) {
    	xLastWakeTime = xTaskGetTickCount();
      fsm_fire(fsm);
      vTaskDelayUntil(&xLastWakeTime, periodTick);
    }

    vTaskDelete(NULL);
}


/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
    PIN_FUNC_SELECT(GPIO_PIN_REG_15, FUNC_GPIO15);
    xTaskCreate(&inicio, "startup", 2048, NULL, 1, NULL);
}
