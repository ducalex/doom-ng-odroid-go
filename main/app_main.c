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

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "odroid.h"

extern int doom_main(int argc, char const * const *argv);
extern void iwad_selector(char **selected_iwad);
extern char *I_DoomSaveDir(void);
extern char *I_DoomExeDir(void);
static char *selected_iwad = NULL;

void doomEngineTask(void *pvParameters)
{
	if (selected_iwad == NULL) {
		char const *argv[] = {"doom", "-cout", "ICWEFDA", NULL};
		doom_main(3, argv);
	} else {
		char const *argv[] = {"doom", "-cout", "ICWEFDA", "-iwad", selected_iwad, NULL};
		doom_main(5, argv);
	}
}

void app_main()
{
	printf("app_main(): Initializing Odroid GO stuff\n");
	odroid_system_init();

	// Create required directories
	odroid_sdcard_mkdir(I_DoomSaveDir());
	odroid_sdcard_mkdir(I_DoomExeDir());

	printf("app_main(): IWad selector\n");
	iwad_selector(&selected_iwad);
	
	printf("app_main(): Starting Doom!\n");
	xTaskCreatePinnedToCore(&doomEngineTask, "doomEngine", 18000, NULL, 5, NULL, 0);
}
