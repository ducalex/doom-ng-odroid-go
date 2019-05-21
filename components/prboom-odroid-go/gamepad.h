#ifndef GAMEPAD_H
#define GAMEPAD_H

#include <stdint.h>

#define ODROID_GAMEPAD_IO_X ADC1_CHANNEL_6
#define ODROID_GAMEPAD_IO_Y ADC1_CHANNEL_7
#define ODROID_GAMEPAD_IO_SELECT GPIO_NUM_27
#define ODROID_GAMEPAD_IO_START GPIO_NUM_39
#define ODROID_GAMEPAD_IO_A GPIO_NUM_32
#define ODROID_GAMEPAD_IO_B GPIO_NUM_33
#define ODROID_GAMEPAD_IO_MENU GPIO_NUM_13
#define ODROID_GAMEPAD_IO_VOLUME GPIO_NUM_0

enum
{
	ODROID_INPUT_UP = 0,
    ODROID_INPUT_RIGHT,
    ODROID_INPUT_DOWN,
    ODROID_INPUT_LEFT,
    ODROID_INPUT_SELECT,
    ODROID_INPUT_START,
    ODROID_INPUT_A,
    ODROID_INPUT_B,
    ODROID_INPUT_MENU,
    ODROID_INPUT_VOLUME,

	ODROID_INPUT_MAX
};

typedef struct
{
    uint8_t values[ODROID_INPUT_MAX];
    uint8_t previous[ODROID_INPUT_MAX];
    uint8_t debounce[ODROID_INPUT_MAX];
    uint8_t realtime[ODROID_INPUT_MAX];
} odroid_gamepad_state;

void odroid_input_read_raw(uint8_t *);
void gamepadInit(void);
void gamepadPoll(void);
uint8_t *gamepadReadChanged();

volatile odroid_gamepad_state gamepad_state;
volatile int joyVal;
#endif