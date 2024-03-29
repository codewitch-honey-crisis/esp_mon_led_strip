// BEGIN CONFIG ///////

// the type of neopixes. can be ws2812, sk6812 or apa106
#define LED_TYPE ws2812
// The data pin for neopixel display
#define LED_PIN 18
// the physical horizontal resolution of the display
#define LED_HRES 8
// the physical vertical resolution of the display
#define LED_VRES 32
// 1 = swap x and y coordinates. width becomes height and vice versa
#define LED_SWAP_XY 1
// 1 = the LEDS are laid out in a single strip in series such that every other row's columns are inverted
#define LED_SNAKE_LAYOUT 1

// END CONFIG ///////

#include <Arduino.h>
#include <rmt_led_strip.hpp>
#include <gfx.hpp>
#include "neopixel_panel.hpp"
#include "interface.hpp"
using rgbw32 = gfx::rgbw_pixel<32>;
using hsl24 = gfx::hsl_pixel<24>;
using hsva32 = gfx::hsva_pixel<32>;
using colorw32 = gfx::color<rgbw32>;

#if LED_SWAP_XY
#define LED_WIDTH LED_VRES
#define LED_HEIGHT LED_HRES
#else
#define LED_WIDTH LED_HRES
#define LED_HEIGHT LED_VRES
#endif
#define LED_COUNT (LED_WIDTH*LED_HEIGHT)

using namespace gfx;
using namespace arduino;

LED_TYPE leds(LED_PIN,LED_COUNT);

neopixel_panel panel(leds,LED_HRES,LED_SWAP_XY,LED_SNAKE_LAYOUT);
using buffer_t = data::circular_buffer<uint8_t,LED_WIDTH>;
buffer_t points;
void setup() {
    Serial.begin(115200);
    panel.initialize();
}
void loop() {
    static uint32_t ts = millis();
    bool draw=false;
    if(millis()>ts+1000) {
        ts = millis();
        points.clear();
        panel.suspend();
        panel.clear(panel.bounds());
        panel.resume();
    }
    int cmd = Serial.read();
    if(cmd==1) {
        ts = millis();
        read_status status;
        if(sizeof(status)==Serial.read((uint8_t*)&status,sizeof(status))) {
            if(points.full()) {
                uint8_t pt;
                points.get(&pt);
            }
            points.put((uint8_t)status.gpu_usage);
            draw = true;
        } else {
            while(Serial.read()!=-1);
        }
    }
    if(draw) {
        panel.suspend();
        panel.clear(panel.bounds());
        size_t x = LED_WIDTH-points.size();
        for(int i = 0;i<points.size();++i) {
            uint8_t pt = *points.peek(i);
            float v = (((float)pt)/100.0f);
            uint16_t y = v*(LED_HEIGHT-1);
            hsva32 px = color<hsva32>::red;
            hsva32 px2 = color<hsva32>::green;
            auto h1 = px.channel<channel_name::H>();
            auto h2 = px2.channel<channel_name::H>();
            // adjust so we don't overshoot
            h2 -= 40;
            // the actual range we're drawing
            auto range = abs(h2 - h1) + 1;
            for(int yy = 0;yy<=pt;++yy) {
                float f = ((float)yy/100.0f);
                px.channel<channel_name::H>(h1 + (range - (f * range)));
                draw::point(panel,point16(x,LED_HEIGHT-f*(LED_HEIGHT-1)-1),px);
            }
            ++x;
        }
        panel.resume();
    }
}
