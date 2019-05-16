// Copyright 2016-2017 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "esp_attr.h"

#include "rom/cache.h"
#include "rom/ets_sys.h"
//#include "rom/spi_flash.h"
#include "rom/crc.h"

#include "soc/soc.h"
#include "soc/dport_reg.h"
#include "soc/io_mux_reg.h"
#include "soc/efuse_reg.h"
#include "soc/rtc_cntl_reg.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>
#include "esp_err.h"
#include "nvs_flash.h"
//#include "esp_partition.h"

//#include "i_system.h"

//#include "spi_lcd.h"


extern void gamepadInit();
extern void Init_SD();
extern int doom_main(int argc, char const * const *argv);
extern void spi_lcd_init() ;

void doomEngineTask(void *pvParameters)
{
    char const *argv[]={"doom","-cout","ICWEFDA", NULL};
    doom_main(3, argv);
}

void app_main()
{
	printf("app_main(): Setting up GPIOs\n");
	gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
	gpio_set_level(GPIO_NUM_2, 0);
	
	printf("app_main(): Initializing SPI LCD\n");
	spi_lcd_init();
	
	printf("app_main(): Initializing SD Card\n");
	Init_SD();
  	
	printf("app_main(): Initializing NVS Storage\n");
	nvs_flash_init();

	printf("app_main(): Initializing gamepad\n");
	gamepadInit();
	
	printf("app_main(): Starting Doom!\n");
	xTaskCreatePinnedToCore(&doomEngineTask, "doomEngine", 18000, NULL, 5, NULL, 0);
}
