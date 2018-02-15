#include "esp_common.h"
#include "freertos/task.h"
#include "gpio.h"
#include "fsm.h"

/* Declaración de variables de sistema */

#define periodTick 100/portTICK_RATE_MS
#define tiempoGuarda 120/portTICK_RATE_MS
#define tiempoPulse 1000/portTICK_RATE_MS
volatile int timeout0 = 0;
volatile int timeout1 = 0;
enum fsm_state {
  ARMADA, DESARMADA,
  EN0, EN1, EN2,
  DIS0, DIS1, DIS2
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

/* Activa la alarma  */
int button_pressed(fsm_t*this) {
  if(!GPIO_INPUT_GET(0)){
    if (xTaskGetTickCount () < timeout0) {
  		timeout0 = xTaskGetTickCount () + tiempoGuarda ;
      timeout1 = xTaskGetTickCount () + tiempoPulse ;
  		return 0;
  	}
  	timeout0 = xTaskGetTickCount () + tiempoGuarda ;
    timeout1 = xTaskGetTickCount () + tiempoPulse ;
    return 1;
  }
  return 0;
}
/* Timeout */
int tmr_1s(fsm_t*this){
  if (xTaskGetTickCount () >= timeout1) {
    return 1;
  }else
    return 0;
}

/* Detecta presencia  */
int presencia(fsm_t*this) {
  return GPIO_INPUT_GET(15);
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
  {DESARMADA,button_pressed,EN0,led_d},
  {EN0,button_pressed,EN1,led_d},
  {EN0,tmr_1s,DESARMADA,led_d},
  {EN1,button_pressed,EN2,led_d},
  {EN1,tmr_1s,DESARMADA,led_d},
  {EN2,button_pressed,DESARMADA,led_d},
  {EN2,tmr_1s,ARMADA,led_d},
  {ARMADA,button_pressed,DIS0,NULL},
  {ARMADA,presencia,ARMADA,led_a},
  {DIS0,button_pressed,DIS1,NULL},
  {DIS0,tmr_1s,ARMADA,NULL},
  {DIS1,button_pressed,DIS2,NULL},
  {DIS1,tmr_1s,ARMADA,NULL},
  {DIS2,button_pressed,ARMADA,NULL},
  {DIS2,tmr_1s,DESARMADA,led_d},
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
