/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *  Misc system stuff needed by Doom, implemented for Linux.
 *  Mainly timer handling, and ENDOOM/ENDBOOM.
 *
 *-----------------------------------------------------------------------------
 */

#include <stdio.h>

#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#ifdef _MSC_VER
#define    F_OK    0    /* Check for file existence */
#define    W_OK    2    /* Check for write permission */
#define    R_OK    4    /* Check for read permission */
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#endif
#include <sys/stat.h>



#include "config.h"
#include <unistd.h>
#include <sched.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#include "m_argv.h"
#include "lprintf.h"
#include "doomtype.h"
#include "doomdef.h"
#include "m_fixed.h"
#include "r_fps.h"
#include "i_system.h"
#include "i_joy.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "esp_timer.h"

#ifdef __GNUG__
#pragma implementation "i_system.h"
#endif

#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS 22

extern SemaphoreHandle_t dispLock;
//SemaphoreHandle_t dmaChannel2Sem;

static bool init_SD = false;
static int nextHandle = 0;
static int displaytime = 0;

typedef struct {
	int ifd;
	void *addr;
	int offset;
	size_t len;
	int used;
} MmapHandle;

#define NO_MMAP_HANDLES 128
static MmapHandle mmapHandle[NO_MMAP_HANDLES];



static unsigned long getMsTicks()
{
  return esp_timer_get_time() / 1000;
}

int I_GetTime_RealTime(void)
{
  return ((esp_timer_get_time() * TICRATE) / 1000000);

}

fixed_t I_GetTimeFrac(void)
{
  unsigned long now;
  fixed_t frac;

  now = getMsTicks();

  if (tic_vars.step == 0)
    return FRACUNIT;
  else
  {
    frac = (fixed_t)((now - tic_vars.start + displaytime) * FRACUNIT / tic_vars.step);
    if (frac < 0)
      frac = 0;
    if (frac > FRACUNIT)
      frac = FRACUNIT;
    return frac;
  }
}

void I_GetTime_SaveMS(void)
{
  if (!movement_smooth)
    return;

  tic_vars.start = getMsTicks();
  tic_vars.next = (unsigned int) ((tic_vars.start * tic_vars.msec + 1.0f) / tic_vars.msec);
  tic_vars.step = tic_vars.next - tic_vars.start;
}

unsigned long I_GetRandomTimeSeed(void)
{
	return 4; //per https://xkcd.com/221/
}

const char* I_GetVersionString(char* buf, size_t sz)
{
  sprintf(buf,"%s v%s (http://prboom.sourceforge.net/)",PACKAGE,VERSION);
  return buf;
}

const char* I_SigString(char* buf, size_t sz, int signum)
{
  return buf;
}

void I_uSleep(unsigned long usecs)
{
	vTaskDelay(usecs/1000);
}

void I_SetAffinityMask(void)
{
}

const char *I_DoomExeDir(void)
{
  return "/sdcard/roms/doom";
}

const char *I_DoomSaveDir(void)
{
  return "/sdcard/odroid/data/doom";
}


void Init_SD()
{
	if (init_SD == true) return;

	xSemaphoreTake(dispLock, portMAX_DELAY);

	sdmmc_host_t host = SDSPI_HOST_DEFAULT();
	host.slot = HSPI_HOST; // HSPI_HOST;
	host.max_freq_khz = SDMMC_FREQ_DEFAULT;

	//host.command_timeout_ms=200;
	//host.max_freq_khz = SDMMC_FREQ_PROBING;
	sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
	slot_config.gpio_miso = PIN_NUM_MISO;
	slot_config.gpio_mosi = PIN_NUM_MOSI;
	slot_config.gpio_sck  = PIN_NUM_CLK;
	slot_config.gpio_cs   = PIN_NUM_CS;
	//slot_config.dma_channel = 1; //2

	esp_vfs_fat_sdmmc_mount_config_t mount_config = {
			.format_if_mount_failed = false,
			.max_files = 8
	};

	sdmmc_card_t* card;
	esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

	if (ret != ESP_OK) {
			if (ret == ESP_FAIL) {
					lprintf(LO_INFO, "Init_SD: Failed to mount filesystem.\n");
			} else {
					lprintf(LO_INFO, "Init_SD: Failed to initialize the card. %d\n", ret);
			}
			return;
	}

	// Create the save dir. It will fail silently if it exists
	mkdir(I_DoomSaveDir(), 0755);

	//sdmmc_card_print_info(stdout, card);
	init_SD = true;

	xSemaphoreGive(dispLock);

	lprintf(LO_INFO, "Init_SD: SD card opened.\n");
}


int I_Open(const char *fname, int flags)
{
	if (init_SD == false)
		Init_SD();

	lprintf(LO_INFO, "I_Open: Opening File: %s\n", fname);
	
	xSemaphoreTake(dispLock, portMAX_DELAY);
	//char filepath[256];
	//sprintf(filepath, "%s/%s", I_DoomExeDir(), fname);
	//FILE *handle = fopen(filepath, "rb");
	FILE *handle = fopen(fname, "rb");
	xSemaphoreGive(dispLock);

	if (!handle) {
		lprintf(LO_INFO, "I_Open: open %s failed\n", fname);	
		return -1;
	}

	return handle;
}


void I_Read(int handle, void* vbuf, size_t sz)
{
	xSemaphoreTake(dispLock, portMAX_DELAY);
	
	for (int i = 0; i < 20; i++) {
		if (fread(vbuf, sz, 1, handle) == 1) {
			xSemaphoreGive(dispLock);
			return;
		}
		lprintf(LO_INFO, "Error Reading %d bytes\n", (int)sz);
	}
	
	xSemaphoreGive(dispLock);

	I_Error("I_Read: Error Reading %d bytes after 20 tries", (int)sz);
}


int I_Lseek(int handle, off_t offset, int whence)
{
	xSemaphoreTake(dispLock, portMAX_DELAY);
	int seeked = fseek(handle, offset, whence);
	xSemaphoreGive(dispLock);
	
	return seeked;
}


int I_Filelength(int handle)
{
  struct stat fileinfo;
  if (fstat(handle, &fileinfo) == -1)
    I_Error("I_Filelength: %s", strerror(errno));
  return fileinfo.st_size;
}


void I_Close(int handle)
{
	lprintf(LO_INFO, "I_Open: Closing File: %d\n", handle);
	
	xSemaphoreTake(dispLock, portMAX_DELAY);
	fclose(handle);
	xSemaphoreGive(dispLock);
}


char* I_FindFile(const char* fname, const char* ext)
{
	char filepath[256];
	char *ret = NULL;

	xSemaphoreTake(dispLock, portMAX_DELAY);
	sprintf(filepath, "%s/%s", I_DoomExeDir(), fname);
	lprintf(LO_INFO, "Looking for file: %s...", fname);
	if (access( filepath, R_OK ) != -1) {
		lprintf(LO_INFO, "Found file: %s\n", filepath);
		ret = strdup(filepath);
	} else {
		lprintf(LO_INFO, "Not found.\n");
	}
	xSemaphoreGive(dispLock);
	
	return ret;
}


// The Odroid GO supports standard file system functions (fopen/fread/etc)
// But because of the shared bus one MUST lock the screen before doing any SD access.

void I_BeginDiskAccess(void)
{
	lprintf(LO_INFO, "I_BeginDiskAccess: locking display\n");
	xSemaphoreTake(dispLock, portMAX_DELAY);
	gpio_set_level(GPIO_NUM_2, 1);
}


void I_EndDiskAccess(void)
{
	lprintf(LO_INFO, "I_EndDiskAccess: unlocking display\n");
	xSemaphoreGive(dispLock);
	gpio_set_level(GPIO_NUM_2, 0);
}



static int getFreeMMapHandle()
{
//	lprintf(LO_INFO, "getFreeMMapHandle: Get free handle... ");
	int n=NO_MMAP_HANDLES;
	while (mmapHandle[nextHandle].used!=0 && n!=0) {
		nextHandle++;
		if (nextHandle==NO_MMAP_HANDLES) nextHandle=0;
		n--;
	}
	if (n==0) {
		lprintf(LO_ERROR, "getFreeMMapHandle: More mmaps than NO_MMAP_HANDLES!\n");
		exit(0);
	}
	
	if (mmapHandle[nextHandle].addr) {
		//spi_flash_munmap(mmapHandle[nextHandle].handle);
		free(mmapHandle[nextHandle].addr);
		mmapHandle[nextHandle].addr=NULL;
//		printf("mmap: freeing handle %d\n", nextHandle);
	}
	int r=nextHandle;
	nextHandle++;
	if (nextHandle==NO_MMAP_HANDLES) nextHandle=0;
//	lprintf(LO_INFO, "Got: %d\n", r);
	return r;
}


void freeUnusedMmaps(void)
{
	lprintf(LO_INFO, "freeUnusedMmaps...\n");
	for (int i=0; i<NO_MMAP_HANDLES; i++) {
		//Check if handle is not in use but is mapped.
		if (mmapHandle[i].used==0 && mmapHandle[i].addr!=NULL) {
			//spi_flash_munmap(mmapHandle[i].handle);
			free(mmapHandle[i].addr);
			mmapHandle[i].addr=NULL;
			mmapHandle[i].ifd=NULL;
			//printf("Freeing handle %d\n", i);
		}
	}
}


void *I_Mmap(void *addr, size_t length, int prot, int flags, int ifd, off_t offset)
{
	int i;
	esp_err_t err;
	void *retaddr = NULL;

	for (i = 0; i < NO_MMAP_HANDLES; i++) {
		if (mmapHandle[i].offset == offset && mmapHandle[i].len == length && mmapHandle[i].ifd == ifd) {
			mmapHandle[i].used++;
			return mmapHandle[i].addr;
		}
	}

	i = getFreeMMapHandle();

	retaddr = malloc(length);
	if (!retaddr)
	{
		lprintf(LO_ERROR, "I_Mmap: No free address space. Cleaning up unused cached mmaps...\n");
		freeUnusedMmaps();
		retaddr = malloc(length);
	}

	if (retaddr)
	{
		I_Lseek(ifd, offset, SEEK_SET);
		I_Read(ifd, retaddr, length);
		mmapHandle[i].addr = retaddr;
		mmapHandle[i].len = length;
		mmapHandle[i].used = 1;
		mmapHandle[i].offset = offset;
		mmapHandle[i].ifd = ifd;
	} else {
		lprintf(LO_ERROR, "I_Mmap: Can't mmap offset: %d (len=%d)!\n", (int)offset, length);
		return NULL;
	}

	return retaddr;
}


int I_Munmap(void *addr, size_t length)
{
	for (int i = 0; i < NO_MMAP_HANDLES; i++) {
		if (mmapHandle[i].addr == addr && mmapHandle[i].len == length) {
			mmapHandle[i].used--;
			return 0;
		}
	}

	lprintf(LO_ERROR, "I_Mmap: Freeing non-mmapped address/len combo!\n");
	exit(0);
}
