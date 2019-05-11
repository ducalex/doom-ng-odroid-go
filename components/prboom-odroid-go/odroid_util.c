#include "esp_heap_caps.h"

size_t total_free_bytes()
{
  multi_heap_info_t info;
  heap_caps_get_info(&info, MALLOC_CAP_DEFAULT);
  return info.total_free_bytes;
}

#if 0
void odroid_system_led_set(int value)
{
    if (!system_initialized)
    {
        printf("odroid_system_init not called before use.\n");
        abort();
    }

    gpio_set_level(GPIO_NUM_2, value);
}


int odroid_input_battery_level_read
void odroid_system_sleep



size_t odroid_sdcard_copy_file_to_memory(const char* path, void* ptr)
{
    size_t ret = 0;

    if (!isOpen)
    {
        printf("odroid_sdcard_copy_file_to_memory: not open.\n");
    }
    else
    {
        if (!ptr)
        {
            printf("odroid_sdcard_copy_file_to_memory: ptr is null.\n");
        }
        else
        {
            FILE* f = fopen(path, "rb");
            if (f == NULL)
            {
                printf("odroid_sdcard_copy_file_to_memory: fopen failed.\n");
            }
            else
            {
                // copy
                const size_t BLOCK_SIZE = 512;
                while(true)
                {
                    __asm__("memw");
                    size_t count = fread((uint8_t*)ptr + ret, 1, BLOCK_SIZE, f);
                    __asm__("memw");

                    ret += count;

                    if (count < BLOCK_SIZE) break;
                }
            }
        }
    }

    return ret;
}

char* odroid_sdcard_create_savefile_path(const char* base_path, const char* fileName)
{
    char* result = NULL;

    if (!base_path) abort();
    if (!fileName) abort();

    //printf("%s: base_path='%s', fileName='%s'\n", __func__, base_path, fileName);

    // Determine folder
    char* extension = fileName + strlen(fileName); // place at NULL terminator
    while (extension != fileName)
    {
        if (*extension == '.')
        {
            ++extension;
            break;
        }
        --extension;
    }

    if (extension == fileName)
    {
        printf("%s: File extention not found.\n", __func__);
        abort();
    }

    //printf("%s: extension='%s'\n", __func__, extension);

    const char* DATA_PATH = "/odroid/data/";
    const char* SAVE_EXTENSION = ".sav";

    size_t savePathLength = strlen(base_path) + strlen(DATA_PATH) + strlen(extension) + 1 + strlen(fileName) + strlen(SAVE_EXTENSION) + 1;
    char* savePath = malloc(savePathLength);
    if (savePath)
    {
        strcpy(savePath, base_path);
        strcat(savePath, DATA_PATH);
        strcat(savePath, extension);
        strcat(savePath, "/");
        strcat(savePath, fileName);
        strcat(savePath, SAVE_EXTENSION);

        printf("%s: savefile_path='%s'\n", __func__, savePath);

        result = savePath;
    }

    return result;
}

#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "driver/i2s.h"
#include "driver/rtc_io.h"

#define ODROID_VOLUME_LEVEL_COUNT (5)

typedef enum
{
    ODROID_VOLUME_LEVEL0 = 0,
    ODROID_VOLUME_LEVEL1 = 1,
    ODROID_VOLUME_LEVEL2 = 2,
    ODROID_VOLUME_LEVEL3 = 3,
    ODROID_VOLUME_LEVEL4 = 4,

    _ODROID_VOLUME_FILLER = 0xffffffff
} odroid_volume_level;

typedef enum
{
    ODROID_AUDIO_SINK_SPEAKER = 0,
    ODROID_AUDIO_SINK_DAC
} ODROID_AUDIO_SINK;

#define I2S_NUM (I2S_NUM_0)

static int AudioSink = ODROID_AUDIO_SINK_SPEAKER;
static float Volume = 1.0f;
static odroid_volume_level volumeLevel = ODROID_VOLUME_LEVEL3;
static int volumeLevels[] = {0, 125, 250, 500, 1000};
static int audio_sample_rate;


odroid_volume_level odroid_audio_volume_get()
{
    return volumeLevel;
}

void odroid_audio_volume_set(odroid_volume_level value)
{
    if (value >= ODROID_VOLUME_LEVEL_COUNT)
    {
        printf("odroid_audio_volume_set: value out of range (%d)\n", value);
        abort();
    }

    volumeLevel = value;
    Volume = (float)volumeLevels[value] * 0.001f;
}


void odroid_audio_init(ODROID_AUDIO_SINK sink, int sample_rate)
{
    printf("%s: sink=%d, sample_rate=%d\n", __func__, sink, sample_rate);

    AudioSink = sink;
    audio_sample_rate = sample_rate;

    // NOTE: buffer needs to be adjusted per AUDIO_SAMPLE_RATE
    if(AudioSink == ODROID_AUDIO_SINK_SPEAKER)
    {
        i2s_config_t i2s_config = {
            //.mode = I2S_MODE_MASTER | I2S_MODE_TX,                                  // Only TX
            .mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN,
            .sample_rate = audio_sample_rate,
            .bits_per_sample = 16,
            .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,                           //2-channels
            .communication_format = I2S_COMM_FORMAT_I2S_MSB,
            //.communication_format = I2S_COMM_FORMAT_PCM,
            .dma_buf_count = 6,
            //.dma_buf_len = 1472 / 2,  // (368samples * 2ch * 2(short)) = 1472
            .dma_buf_len = 512,  // (416samples * 2ch * 2(short)) = 1664
            .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,                                //Interrupt level 1
            .use_apll = 0 //1
        };

        i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);

        i2s_set_pin(I2S_NUM, NULL);
        i2s_set_dac_mode(/*I2S_DAC_CHANNEL_LEFT_EN*/ I2S_DAC_CHANNEL_BOTH_EN);
    }
    else if (AudioSink == ODROID_AUDIO_SINK_DAC)
    {
        i2s_config_t i2s_config = {
            .mode = I2S_MODE_MASTER | I2S_MODE_TX,                                  // Only TX
            .sample_rate = audio_sample_rate,
            .bits_per_sample = 16,
            .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,                           //2-channels
            .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
            .dma_buf_count = 6,
            //.dma_buf_len = 1472 / 2,  // (368samples * 2ch * 2(short)) = 1472
            .dma_buf_len = 512,  // (416samples * 2ch * 2(short)) = 1664
            .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,                                //Interrupt level 1
            .use_apll = 1
        };

        i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);

        i2s_pin_config_t pin_config = {
            .bck_io_num = 4,
            .ws_io_num = 12,
            .data_out_num = 15,
            .data_in_num = -1                                                       //Not used
        };
        i2s_set_pin(I2S_NUM, &pin_config);


        // Disable internal amp
        esp_err_t err;

        err = gpio_set_direction(GPIO_NUM_25, GPIO_MODE_OUTPUT);
        if (err != ESP_OK)
        {
            abort();
        }

        err = gpio_set_direction(GPIO_NUM_26, GPIO_MODE_DISABLE);
        if (err != ESP_OK)
        {
            abort();
        }

        err = gpio_set_level(GPIO_NUM_25, 0);
        if (err != ESP_OK)
        {
            abort();
        }
    }
    else
    {
        abort();
    }

//    odroid_volume_level level = odroid_settings_Volume_get();
//    odroid_audio_volume_set(level);
}

void odroid_audio_terminate()
{
    i2s_zero_dma_buffer(I2S_NUM);
    i2s_stop(I2S_NUM);

    i2s_start(I2S_NUM);


    esp_err_t err = rtc_gpio_init(GPIO_NUM_25);
    err = rtc_gpio_init(GPIO_NUM_26);
    if (err != ESP_OK)
    {
        abort();
    }

    err = rtc_gpio_set_direction(GPIO_NUM_25, RTC_GPIO_MODE_OUTPUT_ONLY);
    err = rtc_gpio_set_direction(GPIO_NUM_26, RTC_GPIO_MODE_OUTPUT_ONLY);
    if (err != ESP_OK)
    {
        abort();
    }

    err = rtc_gpio_set_level(GPIO_NUM_25, 0);
    err = rtc_gpio_set_level(GPIO_NUM_26, 0);
    if (err != ESP_OK)
    {
        abort();
    }
}

void odroid_audio_submit(short* stereoAudioBuffer, int frameCount)
{
    short currentAudioSampleCount = frameCount * 2;

    if (AudioSink == ODROID_AUDIO_SINK_SPEAKER)
    {
        // Convert for built in DAC
        for (short i = 0; i < currentAudioSampleCount; i += 2)
        {
            uint16_t dac0;
            uint16_t dac1;

            if (Volume == 0.0f)
            {
                // Disable amplifier
                dac0 = 0;
                dac1 = 0;
            }
            else
            {
                // Down mix stero to mono
                int32_t sample = stereoAudioBuffer[i];
                sample += stereoAudioBuffer[i + 1];
                sample >>= 1;

                // Normalize
                const float sn = (float)sample / 0x8000;

                // Scale
                const int magnitude = 127 + 127;
                const float range = magnitude  * sn * Volume;

                // Convert to differential output
                if (range > 127)
                {
                    dac1 = (range - 127);
                    dac0 = 127;
                }
                else if (range < -127)
                {
                    dac1  = (range + 127);
                    dac0 = -127;
                }
                else
                {
                    dac1 = 0;
                    dac0 = range;
                }

                dac0 += 0x80;
                dac1 = 0x80 - dac1;

                dac0 <<= 8;
                dac1 <<= 8;
            }

            stereoAudioBuffer[i] = (int16_t)dac1;
            stereoAudioBuffer[i + 1] = (int16_t)dac0;
        }

        int len = currentAudioSampleCount * sizeof(int16_t);
        int count = i2s_write_bytes(I2S_NUM, (const char *)stereoAudioBuffer, len, portMAX_DELAY);
        if (count != len)
        {
            printf("i2s_write_bytes: count (%d) != len (%d)\n", count, len);
            abort();
        }
    }
    else if (AudioSink == ODROID_AUDIO_SINK_DAC)
    {
        int len = currentAudioSampleCount * sizeof(int16_t);

        for (short i = 0; i < currentAudioSampleCount; ++i)
        {
            int sample = stereoAudioBuffer[i] * Volume;

            if (sample > 32767)
                sample = 32767;
            else if (sample < -32768)
                sample = -32767;

            stereoAudioBuffer[i] = (short)sample;
        }

        int count = i2s_write_bytes(I2S_NUM, (const char *)stereoAudioBuffer, len, portMAX_DELAY);
        if (count != len)
        {
            printf("i2s_write_bytes: count (%d) != len (%d)\n", count, len);
            abort();
        }
    }
    else
    {
        abort();
    }
}

int odroid_audio_sample_rate_get()
{
    return audio_sample_rate;
}
#endif