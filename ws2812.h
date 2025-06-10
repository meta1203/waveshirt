#ifndef WS2812_H
#define WS2812_H

#include <stdint.h>
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

/**
 * NOTE:
 *  Take into consideration if your WS2812 is a RGB or RGBW variant.
 *
 *  If it is RGBW, you need to set IS_RGBW to true and provide 4 bytes per
 *  pixel (Red, Green, Blue, White) and use urgbw_u32().
 *
 *  If it is RGB, set IS_RGBW to false and provide 3 bytes per pixel (Red,
 *  Green, Blue) and use urgb_u32().
 *
 *  When RGBW is used with urgb_u32(), the White channel will be ignored (off).
 *
 */
#define IS_RGBW false
#define NUM_PIXELS 600

#ifndef WS2812_PIN
#ifdef PICO_DEFAULT_WS2812_PIN
#define WS2812_PIN PICO_DEFAULT_WS2812_PIN
#else
// default to pin 2 if the board doesn't have a default WS2812 pin defined
#define WS2812_PIN 28
#endif
#endif

// Check the pin is compatible with the platform
#if WS2812_PIN >= NUM_BANK0_GPIOS
#error Attempting to use a pin>=32 on a platform that does not support it
#endif

// Define the structure for an RGB color
typedef struct {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} ws2812_color_t;

typedef void (*pattern)(uint len, uint t);

// Initialize the WS2812 LED strip
void ws2812_init();

// execute test patterns
void ws2812_test();

// Set the color of a single LED
void ws2812_set_led(uint16_t index, ws2812_color_t color);

// Update the LED strip to display the set colors
void ws2812_show(void);

// Clear all LEDs (set them to off)
void ws2812_clear(void);

#endif // WS2812_H