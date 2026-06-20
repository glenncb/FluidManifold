#pragma once

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_event.h"
#include "driver/gpio.h"

namespace CPPGPIO
{
    ESP_EVENT_DECLARE_BASE(INPUT_EVENTS);

    class GpioBase
    {
    protected:
        bool _active_low = false;
    }; // GpioBase Class

    class Gpio : public GpioBase
    {
    private:
        gpio_num_t _pin;   // the pin number
        gpio_mode_t _mode; // the pin direction
        // note that you can't read the state of output pins, so store them here in case we attempt to read them
        int _level = 0;  // the current output pin level as there is no easy way to read the output state

        esp_err_t _init(const gpio_mode_t mode, const gpio_num_t pin, const bool activeLow);
        static bool _interrupt_service_installed;

        esp_event_handler_t _event_handle = nullptr;
        static portMUX_TYPE _eventChangeMutex;

        esp_err_t _clearEventHandlers();

        struct interrupt_args
        {
            bool _event_handler_set = false;
            bool _custom_event_handler_set = false;
            bool _queue_enabled = false;
            gpio_num_t _pin;
            esp_event_loop_handle_t _custom_event_loop_handle {nullptr};
            xQueueHandle _queue_handle {nullptr};
        } _interrupt_args;

    public:
        // constructor and init methods common to both input and output
        Gpio(const gpio_mode_t mode, const gpio_num_t pin, const bool activeLow);
        Gpio(const gpio_mode_t mode, const gpio_num_t pin);
        Gpio(void);
        esp_err_t init(const gpio_mode_t mode, const gpio_num_t pin, const bool activeLow);
        esp_err_t init(const gpio_mode_t mode, const gpio_num_t pin);
        

        // GPIO input methods
        int read(void);

        gpio_num_t pinNum(void) { return _pin; };

        esp_err_t enablePullup(void);
        esp_err_t disablePullup(void);
        esp_err_t enablePulldown(void);
        esp_err_t disablePulldown(void);
        esp_err_t enablePullupPulldown(void);
        esp_err_t disablePullupPulldown(void);
        
        esp_err_t enableInterrupt(gpio_int_type_t int_type);
        esp_err_t setEventHandler(esp_event_handler_t Gpio_e_h);
        esp_err_t setEventHandler(esp_event_loop_handle_t Gpio_e_l, esp_event_handler_t Gpio_e_h);
        void setQueueHandle(xQueueHandle Gpio_e_q);
        static void IRAM_ATTR gpio_isr_callback(void* arg);

        // overload conversion to int operator to allow for easy assignment of the read value
        operator int() {
            return read();
        }


        // GPIO output methods
        esp_err_t on(void);
        esp_err_t off(void);
        esp_err_t toggle(void);
        esp_err_t setLevel(int level);

        // overload the assignment operator to set the pin when used on LHS
        Gpio &operator=(int level)
        {
            setLevel(level);
            return *this;
        }

        // overload the assignment operator to copy pin value from rhs to lhs
        // when both are from class Gpio
        Gpio &operator=(Gpio &rhs)
        {
            int level = rhs.read();
            //setLevel(level);
            return *this;
        }

    }; // Gpio Class
} // Gpio Namespace