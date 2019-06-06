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

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "odroid.h"

extern int doom_main(int argc, char const * const *argv);
extern void iwad_selector(char *argv, int *argc);
extern char *I_DoomSaveDir(void);
extern char *I_DoomExeDir(void);

char *doom_argv[16];
int doom_argc = 1;


void doomEngineTask(void *pvParameters)
{
	doom_argv[0] = strdup("doom");
	doom_main(doom_argc, doom_argv);
}

void app_main()
{
	printf("app_main(): Initializing Odroid GO stuff\n");
	odroid_system_init();

	// Create required directories
	odroid_sdcard_mkdir(I_DoomSaveDir());
	odroid_sdcard_mkdir(I_DoomExeDir());

	printf("app_main(): IWad selector\n");
	iwad_selector(doom_argv, &doom_argc);
	
	printf("app_main(): Starting Doom!\n");
	xTaskCreatePinnedToCore(&doomEngineTask, "doomEngine", 18000, NULL, 5, NULL, 0);
}
