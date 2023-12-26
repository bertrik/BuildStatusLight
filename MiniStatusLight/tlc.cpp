#include <Arduino.h>

#include "FastLED.h"
#include "tlc.h"

#define NUM_LEDS    8
#define PIN_LED     D4

static tlc_mode_t tlc_mode = OFF;
static CRGB leds[NUM_LEDS];

void tlc_init(void)
{
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, 0);
    delay(1);

    FastLED.addLeds < WS2812, PIN_LED, RGB > (leds, NUM_LEDS);
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Black;
    }
}

void tlc_set_mode(tlc_mode_t mode)
{
    tlc_mode = mode;
}

void tlc_set_colors(bool top, bool middle, bool bottom)
{
    leds[7] = CRGB::Black;
    leds[6] = CRGB::Black;
    leds[5] = top ? CRGB::Red : CRGB::Black;
    leds[4] = CRGB::Black;
    leds[3] = middle ? CRGB::Orange : CRGB::Black;
    leds[2] = CRGB::Black;
    leds[1] = bottom ? CRGB::Green : CRGB::Black;
    leds[0] = CRGB::Black;
}

void tlc_run(unsigned long ms)
{
    int blink = (millis() / 500) % 2;

    switch (tlc_mode) {
    case OFF:
        tlc_set_colors(false, false, false);
        break;
    case RED:
        tlc_set_colors(true, false, false);
        break;
    case YELLOW:
        tlc_set_colors(false, true, false);
        break;
    case GREEN:
        tlc_set_colors(false, false, true);
        break;
    case FLASH:
        tlc_set_colors(false, blink, false);
        break;
    default:
        break;
    }
    FastLED.show();
}
