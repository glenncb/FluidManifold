#pragma once
/************************************************************************/
/* serled.h - Class / enum / defs and prototypes for WS2812/WS2812b 	*/
/* serial LEDs (NeoPixels).	Simplified implementation of serial LED     */
/* support utilizing the RMT module to send data as a background task.  */
/*----------------------------------------------------------------------*/
/* serLED Class methods:					                            */
/* Init() - init pin and set all LEDs to black		                    */
/* SetColor(l,c) - set led<l> to color <c>			                    */
/* Send() - send serial data to LEDs			                        */
/*----------------------------------------------------------------------*/
/* Modified: Mon Feb  3 08:37:03 MST 2025                               */
/************************************************************************/
#include <Arduino.h>
#include <map>
#include "globals.h"
#include "cppgpio/cppgpio.h"

// maximum number of serial LEDs allowed
#define MAXSLEDS 1
#define BITS_PER_LED 24
#define TOTAL_LED_BITS (MAXSLEDS * BITS_PER_LED)

// Set RMT clock to 10 MHz - 100ns per tick to enable generation of waveforms for WS2812b
#define RMT_FREQ 10000000 


// 5050 versions of the serial LED are GRB ordered but 5mm version is RGB order
#define GRB_ORDER 1

// leveraged from ESP-IDF Arduino RMT example: https://docs.espressif.com/projects/arduino-esp32/en/latest/api/rmt.html#
//
// Note: code assumes series connected WS2812b LEDs chained one
//      after another, each RGB LED has its 24 bit value
//      for color configuration (8b for each color)
//
//      Bits encoded as pulses as follows:
//
//      "0":
//         +-------+              +--
//         |       |              |
//         |       |              |
//         |       |              |
//      ---|       |--------------|
//         +       +              +
//         | 0.4us |   0.85 0us   |
#define RMT_DURATION_HIGH_LOGIC_0 4
#define RMT_DURATION_LOW_LOGIC_0 8
//
//      "1":
//         +-------------+       +--
//         |             |       |
//         |             |       |
//         |             |       |
//         |             |       |
//      ---+             +-------+
//         |    0.8us    | 0.4us |
#define RMT_DURATION_HIGH_LOGIC_1 8
#define RMT_DURATION_LOW_LOGIC_1 4
#define RMT_DURATION_LOW_RESET 20

// predefined (simple) color palette for use with the serial LEDs
typedef enum __attribute__((packed)) { sledBLACK=0, sledRED, sledORANGE, sledYELLOW, sledGREEN, sledBLUE, sledMAGENTA, sledCYAN, sledWHITE } ledcolor_t;

// driver value for selecting between setting color and brightness
#define LEDDRVR_COLOR 0
#define LEDDRVR_BRIGHTNESS 1

class serLED
{
    private: 
        typedef struct __attribute__((packed)) { uint8_t r; uint8_t g; uint8_t b; } ledrgb_t;
        rmt_data_t led_data[TOTAL_LED_BITS]; // the data to send to the LEDs
        // color table is always in r/g/b order, any fixup for ordering is done in the Send routine
        std::map<ledcolor_t, ledrgb_t> ColorTable = {
            {sledBLACK,     {0,     0,      0}},     // black
            {sledRED,       {255,   0,      0}},     // red
            {sledORANGE,    {255,   127,    0}},     // orange
            {sledYELLOW,    {255,   255,    0}},     // yellow
            {sledGREEN,     {0,     255,    0}},     // green
            {sledBLUE,      {0,     0,      255}},   // blue
            {sledMAGENTA,   {255,   0,      255}},   // magenta
            {sledCYAN,      {0,     200,    220}},   // cyan
            {sledWHITE,     {255,   255,    255}}    // white
        };

        int pin; // the pin to use for sending serial data to LEDs
        uint8_t curled; // currently selected LED
        uint8_t brightness; // global brightness level (0..100)
        ledcolor_t led[MAXSLEDS]; // array of color entries per led within the string
        //LiteLED ledstrip{LED_STRIP_WS2812, false, RMT_CHANNEL_0}; // the LED strip object
    public:
        // constructor
        serLED(); 
        /** @brief Initialize LED port, setup the RMT module and send black color value to all LEDs
         *  @param p - the GPIO pin number to use for the LED data
         */
        void Init(int p);
        /** @brief Select the target LED
         *  @param lednum - the LED number to select
         */
        void Select(uint8_t lednum) { if(lednum < MAXSLEDS) curled = lednum; };
        /** @brief set led <lednum> to color <color>
         *  @param lednum - the LED number to set
         *  @param color - the desired color for that LED
         */
        void SetColor(uint8_t lednum, ledcolor_t color) { if(lednum < MAXSLEDS) { curled = lednum; led[curled] = color; }};
        /** @brief set led <curled> to color <color>
         *  @param color - the desired color for that LED
         */
        void SetColor(ledcolor_t color) { led[curled] = color; };
        /** @brief get the current color of the selected LED
         *  @param lednum - the LED number to get the color for
         *  @return the color of the selected LED
         */
        ledcolor_t GetColor(uint8_t lednum) { curled = lednum; return led[curled]; };
        /** @brief get the current color of the selected LED
         *  @return the color of the selected LED
         */
        ledcolor_t GetColor(void ) { return led[curled]; };
        /** @brief set global brightness level (0-100%)
         *  @param pct - brightness level 0-100%
         */
        void Brightness(uint8_t pct) { brightness = (pct>100 ? 100 : pct); };
        /** @brief set global brightness level (0-100%)
         *  @param pct - brightness level 0-100%
         */
        uint8_t GetBrightness(void) { return brightness; };
        /** @brief Send serial data to all the serial LEDs in RGB/GRB format
         */
        void Send(void);
        /** @brief Send serial data to all the serial LEDs in RGB/GRB format
         */
        void Reset(void);
};  

void modr_SERLED(uint16_t *val, uint8_t drvaddr, uint8_t drvchan);
void modw_SERLED(uint16_t *val, uint8_t drvaddr, uint8_t drvchan);

extern serLED sled;