/*
 * This file is part of doom-ng-odroid-go.
 * Copyright (c) 2019 ducalex.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "i_system.h"

#include "odroid.h"

extern void iwad_selector(char *argv, int *argc);

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
