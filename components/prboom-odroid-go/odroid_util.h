#include "esp_heap_caps.h"

size_t free_bytes_total();
size_t free_bytes_internal();
size_t free_bytes_spiram();

void odroid_system_setup();
void odroid_system_led_set(int value);
void odroid_spi_bus_acquire();
void odroid_spi_bus_release();