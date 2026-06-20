/****************************************************************/
/* serled.c - library for managing WS2812/WS2812b serial LEDs	*/
/* Simplified implementation of serial LED support utilizing    */
/* the RMT module to send data as a background task.            */
/*--------------------------------------------------------------*/
/* Public methods:					                            */
/* Init(p) - define init pin <p> and set all LEDs to black		*/
/* SetColor(l,c) - set led<l> to color <c>			            */
/* Send() - send serial data to LEDs			                */
/*--------------------------------------------------------------*/
/* Modified: Mon Feb  3 08:37:03 MST 2025                       */
/****************************************************************/
#include <Arduino.h>
#include "serled.h"
#include "globals.h"
#include "pins.h"


// constructor
serLED::serLED(void)
{ 
    pin = -1; 
    curled = 0; 
    brightness = 25;
    for(int i=0;i<MAXSLEDS;i++) led[i] = sledBLACK; 
};

// associate the pin with serLED and send all pins to black
void serLED::Init(int p) 
{
    if(p < 0) 
    { 
        ESP_LOGE("serLED::Init", "invalid pin number %d", pin);
        return;
    }

    pin = p;

    // initialize the RMT peripheral
    if (!rmtInit(PIN_LEDSI, RMT_TX_MODE, RMT_MEM_NUM_BLOCKS_1, RMT_FREQ)) 
    {
        ESP_LOGE("serLED::Init", "rmtinit sender failed");
    }
    else
    {
        ESP_LOGI("serLED::Init", "rmtinit initialized for pin %d at period %.1fns", PIN_LEDSI, 1000000000/RMT_FREQ);
    }

    // set all pins black
    for(int i=0;i<MAXSLEDS;i++) led[i] = sledBLACK; 

    // reset the LEds
    Reset();
    // send the (black) data to the LEDs
    Send();
}

// load the led_data array with the RMT modulation data for each bit
void serLED::Send(void) 
{
    int i = 0;

#ifdef GRB_ORDER
    typedef struct { uint8_t g; uint8_t r; uint8_t b; } rgbgrb_t;
#else
    typedef struct { uint8_t r; uint8_t g; uint8_t b; } rgbgrb_t;
#endif

    union {
        uint8_t bytes[3];
        rgbgrb_t rgb;
    } colordata;

    for(int l=0;l<MAXSLEDS;l++) 
    {
        ledcolor_t color = led[l];
        colordata.rgb.g = (uint8_t) (ColorTable[color].g * brightness / 100);
        colordata.rgb.r = (uint8_t) (ColorTable[color].r * brightness / 100);
        colordata.rgb.b = (uint8_t) (ColorTable[color].b * brightness / 100);
        //Serial.printf("Sending color #%d to led %d (r=%d, g=%d, b=%d)\n", color, l, colordata.rgb.r, colordata.rgb.g, colordata.rgb.b);
        // process the color data in GRB/RGB order
        //Serial.printf("led %d = ", l);
        for(int c=0; c<3; c++)
        {
            // go through each bit in the color entry and set the corresponding RMT data
            for(int b=0; b<8; b++)
            {
                if(colordata.bytes[c] & (1 << (7-b)))
                {
                    //Serial.printf("1");
                    led_data[i].level0 = 1;
                    led_data[i].duration0 = RMT_DURATION_HIGH_LOGIC_1;
                    led_data[i].level1 = 0;
                    led_data[i].duration1 = RMT_DURATION_LOW_LOGIC_1;
                } 
                else 
                {
                    //Serial.printf("0");
                    led_data[i].level0 = 1;
                    led_data[i].duration0 = RMT_DURATION_HIGH_LOGIC_0;
                    led_data[i].level1 = 0;
                    led_data[i].duration1 = RMT_DURATION_LOW_LOGIC_0;
                }
                i++;
            }
            //Serial.printf(" ");
        }   
        //Serial.printf("\n");
    }

    // send the data to the LEDs
    rmtWrite(PIN_LEDSI, led_data, TOTAL_LED_BITS, RMT_WAIT_FOR_EVER);
}

// send a long pulse to the LEDs to force a reset
void serLED::Reset(void) 
{
    int i = 0;

    led_data[0].level0 = 0;
    led_data[0].duration0 = RMT_DURATION_LOW_RESET;
    led_data[0].level1 = 0;
    led_data[0].duration1 = RMT_DURATION_LOW_RESET;
    // send the data to the LEDs
    rmtWrite(PIN_LEDSI, led_data, 1, RMT_WAIT_FOR_EVER);
}

// modbus drivers
void modr_SERLED(uint16_t *val, uint8_t drvaddr, uint8_t drvchan)
{
  if(!val)
  {
    ESP_LOGE("modr_SERLED", "val pointer is null");
  }

  // otherwise return the requested value
  switch (drvaddr)
  {
#if 0
    ledcolor_t color;
    case LEDDRVR_COLOR: // get current fan setpoint RPM
      color  = sled.GetColor());
      *val = static_cast<uint16_t>(color);
      break;
#endif
    case LEDDRVR_BRIGHTNESS: // get current fan speed (actual RPM)
      *val = sled.GetBrightness();
      break;
    default:
      ESP_LOGE("modr_SERLED", "invalid drvaddr %d", drvaddr);
      break;
  }
  Serial.printf("modr_SERLED addr %d chan %d, val=%d\n", drvaddr, drvchan, *val);
}

void modw_SERLED(uint16_t *val, uint8_t drvaddr, uint8_t drvchan)
{
  if(!val)
  {
    ESP_LOGE("modw_SERLED", "val pointer is null");
    return;
  }

  // otherwise return the requested value
  switch (drvaddr)
  {
    uint8_t bright;
    ledcolor_t color;
#if 0
    case LEDDRVR_COLOR: // set current LED color
      if(*val > sledWHITE)
      {
        ESP_LOGE("modw_SERLED", "invalid color value %d", *val);
        return;
      }
      color = static_cast<ledcolor_t>(*val);
      sled.SetColor(color);
      sled.Send();
      break;
#endif
    case LEDDRVR_BRIGHTNESS: // set current LED brightness
      bright = (*val > 100) ? 100 : *val;
      sled.Brightness(bright);
      sled.Send();
      break;
    default:
      ESP_LOGE("modw_SERLED", "invalid drvaddr %d", drvaddr);
      break;
  }
  return;
}

serLED sled;
