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
 *  System interface for sound.
 *
 *-----------------------------------------------------------------------------
 */

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "config.h"
#include <math.h>
#include <unistd.h>

#include "d_main.h"
#include "z_zone.h"
#include "m_swap.h"
#include "i_sound.h"
#include "m_argv.h"
#include "m_misc.h"
#include "w_wad.h"
#include "lprintf.h"
#include "s_sound.h"
#include "doomdef.h"
#include "doomstat.h"
#include "doomtype.h"
#include "mmus2mid.h"
#include "midifile.h"
#include "oplplayer.h"
#include "nvs.h"

#include "odroid.h"

extern void loadSettings();
extern void saveSettings();

#define SAMPLECOUNT         128
#define SAMPLESIZE          2      // 16bit
#define SAMPLERATE          11025  // Hz
#define MIXBUFFERSIZE       (SAMPLECOUNT * SAMPLESIZE * 2) // Stereo 16bits
#define NUM_MIX_CHANNELS    8

typedef struct {
    unsigned char* data;   // Sample
    unsigned char* endptr; // End of data
    int start;             // Time/gametic that the channel started playing
    int sfxid;             // SFX id of the playing sound effect.
} channel_t;

typedef struct
{
  char           ID[4];            // identifier "MUS"0x1A
  uint16_t       ScoreLength;      // length of music portion
  uint16_t       ScoreStart;       // offset of music portion
  uint16_t       channels;         // count of primary channels
  uint16_t       SecChannels;      // count of secondary channels
  uint16_t       InstrCnt;         // number of instruments
} PACKEDATTR MUSheader;

const music_player_t *music_player = &opl_synth_player;

// The channel data pointers, start and end.
channel_t channels[NUM_MIX_CHANNELS];
unsigned char mixbuffer[MIXBUFFERSIZE] = {0};
int sfx_lengths[NUMSFX];

int snd_card, mus_card, snd_samplerate;
bool musicPlaying = false;

//
// This function loads the sound data from the WAD lump,
//  for single sound.
//
void* getsfx(char* sfxname, int* len)
{
    unsigned char*      sfx;
    unsigned char*      paddedsfx;
    int                 size;
    int                 paddedsize;
    char                name[20];
    int                 sfxlump;

    // Get the sound data from the WAD, allocate lump in zone memory.
    sprintf(name, "ds%s", sfxname);

    // We use dspistol as a default sound
    if (W_CheckNumForName(name) == -1) {
        strcpy(name, "dspistol");
    }

    sfxlump = W_GetNumForName(name);
    size = W_LumpLength(sfxlump);
    sfx = (unsigned char*)W_CacheLumpNum(sfxlump);

    // Pads the sound effect out to the mixing buffer size.
    // The original realloc would interfere with zone memory.
    paddedsize = ((size-8 + (SAMPLECOUNT-1)) / SAMPLECOUNT) * SAMPLECOUNT;

    // Allocate from zone memory.
    paddedsfx = (unsigned char*)Z_Malloc(paddedsize+8, PU_STATIC, 0);

    // Now copy and pad.
    memcpy(paddedsfx, sfx, size);
    memset(paddedsfx+size, 128, paddedsize+8-size);

    *len = paddedsize;

    // Remove the cached lump.
    //Z_Free(sfx);
    W_UnlockLumpNum(sfxlump);

    // Return allocated padded data.
    return (void *) (paddedsfx + 8);
}


void saveAudioSettings()
{
    nvs_handle nvs_h;
    nvs_open("audio", NVS_READWRITE, &nvs_h);
	nvs_set_i32(nvs_h, "master_volume", snd_MasterVolume);
	nvs_set_i32(nvs_h, "music_volume", snd_MusicVolume); // M_LookupDefault("music_volume")->current
	nvs_set_i32(nvs_h, "sfx_volume", snd_SfxVolume);
	nvs_commit(nvs_h);
    nvs_close(nvs_h);
}


void I_UpdateSoundParams(int handle, int volume, int seperation, int pitch)
{
    // Basically, this should propagate
    //  the menu/config file setting
    //  to the state variable used in
    //  the mixing.
}


void I_SetChannels()
{
    // Init internal lookups (raw data, mixing buffer, channels).
    // This function sets up internal lookups used during
    //  the mixing process.
}

// Retrieve the raw data lump index
//  for a given SFX name.
//
int I_GetSfxLumpNum(sfxinfo_t* sfx)
{
    char namebuf[9];
    sprintf(namebuf, "ds%s", sfx->name);
    return W_GetNumForName(namebuf);
}


int I_StartSound(int sfxid, int channel, int vol, int sep, int pitch, int priority)
{
    int oldest = gametic;
    int oldestnum = 0;
    int i, slot;

    // Chainsaw troubles.
    // Play these sound effects only one at a time.
    if ( sfxid == sfx_sawup
        || sfxid == sfx_sawidl
        || sfxid == sfx_sawful
        || sfxid == sfx_sawhit
        || sfxid == sfx_stnmov
        || sfxid == sfx_pistol	 )
    {
        // Loop all channels, check.
        for (i=0 ; i<NUM_MIX_CHANNELS ; i++) {
            // Active, and using the same SFX?
            if ( (channels[i].data) && (channels[i].sfxid == sfxid) ) {
                // Reset.
                channels[i].data = 0;
                break;
            }
        }
    }

    // Loop all channels to find oldest SFX.
    for (i=0; (i<NUM_MIX_CHANNELS) && (channels[i].data); i++)
    {
        if (channels[i].start < oldest)
        {
            oldestnum = i;
            oldest = channels[i].start;
        }
    }

    // Use empty channel if available, otherwise reuse the oldest one
    slot = (i == NUM_MIX_CHANNELS) ? oldestnum : i;
    channels[slot].data = (unsigned char *) S_sfx[sfxid].data;
    channels[slot].endptr = channels[slot].data + sfx_lengths[sfxid];
    channels[slot].sfxid = sfxid;

    return 1;
}


void I_StopSound(int handle)
{

}


int I_SoundIsPlaying(int handle)
{
    return gametic < handle;
}


int I_AnySoundStillPlaying(void)
{
    return false;
}

// This function loops all active (internal) sound
//  channels, retrieves a given number of samples
//  from the raw sound data, modifies it according
//  to the current (internal) channel parameters,
//  mixes the per channel samples into the global
//  mixbuffer, clamping it to the allowed range,
//  and sets up everything for transferring the
//  contents of the mixbuffer to the (two)
//  hardware channels (left and right, that is).
//
// This function currently supports only 16bit.
//
void IRAM_ATTR I_UpdateSound( void )
{
    // Pointers in global mixbuffer, left, right, end.
    //unsigned char*		leftout;
    //unsigned char*		rightout;
    short* audioBuffer = (short*)mixbuffer;

    // Mix sounds into the mixing buffer.
    while (audioBuffer < (mixbuffer+SAMPLECOUNT*SAMPLESIZE))
    {
        int totalSample = 0; // float
        int totalChannelCount = 0;

        if (snd_SfxVolume > 0) {
            for (int chan = 0; chan < NUM_MIX_CHANNELS; chan++)
            {
                // Check channel, if active.
                if (channels[chan].data)
                {
                    // Get the raw data from the channel.
                    if (*channels[chan].data != 0) {
                        totalSample += *channels[chan].data;
                        ++totalChannelCount;
                    }

                    channels[chan].data++;

                    // Check whether we are done.
                    if (channels[chan].data >= channels[chan].endptr) {
                        channels[chan].data = 0;
                    }
                }
            }

            // Adjust for sfx volume
            totalSample /= (16 - snd_SfxVolume);
        }

        if (musicPlaying && snd_MusicVolume > 0) {
            int16_t stream[2] = {0};
            music_player->render(&stream, 1); // It returns 2 (stereo) 16bits values per sample
            if (stream[1] < 0) stream[1] = 0; // drop negative values, our DAC doesn't like that

            // drop a few bits otherwise it's too loud, same as dividing by 16
            totalSample += (stream[1] >> 4) / (16 - snd_MusicVolume); // float

            if (totalChannelCount == 0) {
                totalChannelCount = 1;
            }
        }

        if (totalChannelCount == 0 || snd_MasterVolume == 0)
        {
            *(audioBuffer++) = 0;
            *(audioBuffer++) = 0;
        }
        else
        {
            *(audioBuffer++) = 0x8000;
            *(audioBuffer++) = ((short)((totalSample / totalChannelCount) / (16 - snd_MasterVolume)) << 8);
        }
    }
}


void I_ShutdownSound(void)
{
    odroid_sound_deinit();
}


void IRAM_ATTR updateTask(void *arg)
{
    while(1)
    {
        I_UpdateSound();
        odroid_sound_write(mixbuffer, 256);
    }
}


void I_InitSound(void)
{
    odroid_sound_set_sample_rate(SAMPLERATE);
    memset(mixbuffer, 0, MIXBUFFERSIZE);

    // Initialize external data (all sounds) at start, keep static.
    lprintf(LO_INFO, "I_InitSound: ");

    for (int i = 1; i < NUMSFX; i++) {
        // Alias? Example is the chaingun sound linked to pistol.
        if (!S_sfx[i].link) {
            // Load data from WAD file.
            S_sfx[i].data = getsfx( S_sfx[i].name, &sfx_lengths[i] );
        } else {
            // Previously loaded already?
            S_sfx[i].data = S_sfx[i].link->data;
            sfx_lengths[i] = sfx_lengths[(S_sfx[i].link - S_sfx)/sizeof(sfxinfo_t)];
        }
    }

    lprintf( LO_INFO, " pre-cached all sound data\n");

    // Load volume settings from NVS
    nvs_handle nvs_h;
    nvs_open("audio", NVS_READWRITE, &nvs_h);
	if (nvs_get_i32(nvs_h, "master_volume", &snd_MasterVolume) != ESP_OK) {
		snd_MasterVolume = 9;
	}
	if (nvs_get_i32(nvs_h, "music_volume", &snd_MusicVolume) != ESP_OK) {
		snd_MusicVolume = 15;
	}
	if (nvs_get_i32(nvs_h, "sfx_volume", &snd_SfxVolume) != ESP_OK) {
		snd_SfxVolume = 15;
	}
    nvs_close(nvs_h);

    music_player->init(SAMPLERATE);
    music_player->setvolume(9);

    // Finished initialization.
    lprintf(LO_INFO, "I_InitSound: sound module ready\n");

    xTaskCreatePinnedToCore(&updateTask, "updateTask", 1000, NULL, 6, NULL, 1);
}


void I_ShutdownMusic(void)
{
    lprintf(LO_INFO, "I_ShutdownMusic: called\n");
    music_player->shutdown();
}


void I_InitMusic(void)
{
    lprintf(LO_INFO, "I_InitMusic: called\n");
    //music_player->init(SAMPLERATE * 2);
}


void I_PlaySong(int handle, int looping)
{
    lprintf(LO_INFO, "I_PlaySong: %d %d\n", handle, looping);
    music_player->play(handle, looping);
    musicPlaying = true;
}


void I_PauseSong(int handle)
{
    lprintf(LO_INFO, "I_PauseSong: handle: %d.\n", handle);
    music_player->pause();
    musicPlaying = false;
}


void I_ResumeSong(int handle)
{
    lprintf(LO_INFO, "I_ResumeSong: handle: %d.\n", handle);
    music_player->resume();
    musicPlaying = true;
}


void I_StopSong(int handle)
{
    lprintf(LO_INFO, "I_StopSong: handle: %d.\n", handle);
    music_player->stop();
    musicPlaying = false;
}


void I_UnRegisterSong(int handle)
{
    size_t start_mem = free_bytes_total();

    lprintf(LO_INFO, "I_UnRegisterSong: handle: %d\n", handle);
    music_player->unregistersong(handle);
    lprintf(LO_INFO, "I_UnRegisterSong: mem freed: %d\n", free_bytes_total() - start_mem);
}


int I_RegisterSong(const void *data, size_t len)
{
    size_t start_mem = free_bytes_total();

    static MUSheader MUSh;
    void *music_handle = NULL;
    MIDI mididata;
    int midlen;
    uint8_t *mid;

    memcpy(&MUSh, data, sizeof(MUSheader));

    lprintf(LO_INFO, "I_RegisterSong: Length: %d, Start: %d, Channels: %d, SecChannels: %d, Instruments: %d.\n",
                                MUSh.ScoreLength, MUSh.ScoreStart, MUSh.channels, MUSh.SecChannels, MUSh.InstrCnt);

    if (mmus2mid(data, &mididata, 64, 1) != 0) {
        return 0;
    }

    if (MIDIToMidi(&mididata, &mid, &midlen) == 0) {
        free_mididata(&mididata);
        music_handle = music_player->registersong(mid, midlen);
        free(mid);
    }

    lprintf(LO_INFO, "I_RegisterSong: mem used: %d\n", start_mem - free_bytes_total());

    return music_handle;
}


int I_RegisterMusic(const char* filename, musicinfo_t *song)
{
    lprintf(LO_INFO, "I_RegisterMusic: %s\n", filename);
    return 1;
}


void I_SetMusicVolume(int volume)
{
    //music_player->setvolume(volume);
    saveAudioSettings();
}
