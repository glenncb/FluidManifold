// file: cppgpio.cpp
// author: Glenn CB (glenncb@phasethreedev.com
// created: 2024-02-17
// copyright: (c) 2024 Phase Three Product Development
// description: cppgpio library for ESP32 - class definition to configure and access GPIOs
// Note: this library is a modified version of the code from this article: https://embeddedtutorials.com/eps32/esp32-gpio-in-cpp-part-3/
// Which is available on github here: https://github.com/ravisha2396/CPPGPIO.git
// Modifications:
// 1. combined the input and output classes into a single class, Gpio
// 2. added operator overloading to allow the Gpio class to be used in either the rhs or lhs of an assignment, or both
// 3. added serial console output for debugging (most of these are commented out unless there is an error)

#include <Arduino.h>
#include "cppgpio/cppgpio.h"
#include "esp_log.h"
#include "esp_err.h"

namespace CPPGPIO
{
    // Static variable initializations
    bool Gpio::_interrupt_service_installed{false};
    portMUX_TYPE Gpio::_eventChangeMutex = portMUX_INITIALIZER_UNLOCKED;

    ESP_EVENT_DEFINE_BASE(INPUT_EVENTS);

    // GPIO ISR callback for interrupt on change 
    void IRAM_ATTR Gpio::gpio_isr_callback(void *args)
    {
        int32_t pin = (reinterpret_cast<struct interrupt_args *>(args))->_pin;

        bool custom_event_handler_set = (reinterpret_cast<struct interrupt_args *>(args))->_custom_event_handler_set;
        bool event_handler_set = (reinterpret_cast<struct interrupt_args *>(args))->_event_handler_set;
        bool queue_enabled = (reinterpret_cast<struct interrupt_args *>(args))->_queue_enabled;
        esp_event_loop_handle_t custom_event_loop_handle = (reinterpret_cast<struct interrupt_args *>(args))->_custom_event_loop_handle;
        xQueueHandle queue_handle = (reinterpret_cast<struct interrupt_args *>(args))->_queue_handle;

        esp_err_t status {ESP_OK};

        // select the method to use to post the event
        if (queue_enabled) // event queue is highest priority
        {
            //Serial.println("Sending GPIO ISR event to user task queue, xQueueSendFromISR()");
            xQueueSendFromISR(queue_handle, &pin, NULL);
        }
        else if (custom_event_handler_set) // custom event loop is next highest priority
        {
            //Serial.println("Posting GPIO ISR event to custom event loop, esp_event_isr_post_to()");
            esp_event_isr_post_to(custom_event_loop_handle, INPUT_EVENTS, pin, nullptr, 0, nullptr);
        }
        else if (event_handler_set) // otherwise, default to the system event loop
        {
            //Serial.println("Posting GPIO ISR event to default system loop, esp_event_isr_post()");
            esp_event_isr_post(INPUT_EVENTS, pin, nullptr, 0, nullptr);
        }
    }

    // GPIO initialization / constructor methods common to both input and output
    esp_err_t Gpio::_init(const gpio_mode_t mode, const gpio_num_t pin, const bool activeLow)
    {
        esp_err_t status{ESP_OK};

        // save pin information to private variables
        _active_low = activeLow;
        _interrupt_args._pin = pin; // FIXME - this is a duplicate of the pin number in _pin
        _pin = pin;
        _mode = mode;

        gpio_config_t cfg;
        cfg.pin_bit_mask = 1ULL << pin;
        cfg.mode = mode;
        cfg.pull_up_en = GPIO_PULLUP_DISABLE;
        cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
        cfg.intr_type = GPIO_INTR_DISABLE;

        status |= gpio_config(&cfg);

        return status;
    }

    Gpio::Gpio(const gpio_mode_t mode, const gpio_num_t pin, const bool activeLow)
    {
        _init(mode, pin, activeLow);
    }

    Gpio::Gpio(const gpio_mode_t mode, const gpio_num_t pin)
    {
        _init(mode, pin, false);
    }

    Gpio::Gpio(void)
    {
    }

    esp_err_t Gpio::init(const gpio_mode_t mode, const gpio_num_t pin, const bool activeLow)
    {
        return _init(mode, pin, activeLow);
    }

    esp_err_t Gpio::init(const gpio_mode_t mode, const gpio_num_t pin)
    {
        return _init(mode, pin, false);
    }

    // GPIO input methods
    int Gpio::read(void)
    {
        // handle special case of reading from an output
        if ((_mode == GPIO_MODE_OUTPUT) || (_mode == GPIO_MODE_OUTPUT_OD))
        {
            return _active_low ? !_level : _level; // use the locally stored level value
        }
        else // otherwise just read the current input pin level
        {
            return _active_low ? !gpio_get_level(_interrupt_args._pin) : gpio_get_level(_interrupt_args._pin);
        }
    }

    esp_err_t Gpio::enablePullup(void)
    {
        return gpio_set_pull_mode(_interrupt_args._pin, GPIO_PULLUP_ONLY);
    }

    esp_err_t Gpio::disablePullup(void)
    {
        return gpio_set_pull_mode(_interrupt_args._pin, GPIO_FLOATING);
    }

    esp_err_t Gpio::enablePulldown(void)
    {
        return gpio_set_pull_mode(_interrupt_args._pin, GPIO_PULLDOWN_ONLY);
    }

    esp_err_t Gpio::disablePulldown(void)
    {
        return gpio_set_pull_mode(_interrupt_args._pin, GPIO_FLOATING);
    }

    esp_err_t Gpio::enablePullupPulldown(void)
    {
        return gpio_set_pull_mode(_interrupt_args._pin, GPIO_PULLUP_PULLDOWN);
    }

    esp_err_t Gpio::disablePullupPulldown(void)
    {
        return gpio_set_pull_mode(_interrupt_args._pin, GPIO_FLOATING);
    }

    esp_err_t Gpio::enableInterrupt(gpio_int_type_t int_type)
    {
        esp_err_t status{ESP_OK};

        // Invert triggers if active low is enabled
        if (_active_low)
        {
            switch (int_type)
            {
            case GPIO_INTR_POSEDGE:
                int_type = GPIO_INTR_NEGEDGE;
                break;
            case GPIO_INTR_NEGEDGE:
                int_type = GPIO_INTR_POSEDGE;
                break;
            case GPIO_INTR_LOW_LEVEL:
                int_type = GPIO_INTR_HIGH_LEVEL;
                break;
            case GPIO_INTR_HIGH_LEVEL:
                int_type = GPIO_INTR_LOW_LEVEL;
                break;
            default:
                break;
            }
        }

        if (!_interrupt_service_installed)
        {
            status = gpio_install_isr_service(0);
            if (ESP_OK == status)
            {
                _interrupt_service_installed = true;
            }            
            else
            {
                Serial.printf("gpio_install_isr_service() failed! Error = %s\n", esp_err_to_name(status));
            }
        }

        if (ESP_OK == status)
        {
            status = gpio_set_intr_type(_interrupt_args._pin, int_type);
            if(status != ESP_OK)
            {
                Serial.printf("gpio_set_intr_type() failed! Error = %s\n", esp_err_to_name(status));
            }
        }

        if (ESP_OK == status)
        {
            status = gpio_isr_handler_add(_interrupt_args._pin, gpio_isr_callback, (void*) &_interrupt_args);
            if(status != ESP_OK)
            {
                Serial.printf("gpio_isr_handler_add() failed! Error = %s\n", esp_err_to_name(status));
            }
        }
        return status;
    }

    esp_err_t Gpio::setEventHandler(esp_event_handler_t Gpio_e_h)
    {
        esp_err_t status{ESP_OK};

        //Serial.println("setEventHandler() invoked.");
        // use taskENTER_CRITICAL and taskEXIT_CRITICAL to protect the event handler from being changed while it is being used
        // minimize the code between these calls
        taskENTER_CRITICAL(&_eventChangeMutex);

        status = _clearEventHandlers();

        status = esp_event_handler_instance_register(INPUT_EVENTS, _interrupt_args._pin, Gpio_e_h, 0, nullptr);

        if (ESP_OK == status)
        {
            //Serial.println("Event handler registered.");
            _interrupt_args._event_handler_set = true;
        }
        else
        {
            Serial.printf("Gpio::setEventHandler: Event handler failed to register! Error = %s\n", esp_err_to_name(status));
        }

        taskEXIT_CRITICAL(&_eventChangeMutex);

        return status;
    }

    // alternate method to set an event handler with a custom event loop instead of the system default event loop handler
    esp_err_t Gpio::setEventHandler(esp_event_loop_handle_t Gpio_e_l, esp_event_handler_t Gpio_e_h)
    {
        esp_err_t status{ESP_OK};

        taskENTER_CRITICAL(&_eventChangeMutex);

        status = _clearEventHandlers();

        status |= esp_event_handler_instance_register_with(Gpio_e_l, INPUT_EVENTS, _interrupt_args._pin, Gpio_e_h, 0, nullptr);

        if (ESP_OK == status)
        {
            _event_handle = Gpio_e_h;
            _interrupt_args._custom_event_loop_handle = Gpio_e_l;
            _interrupt_args._custom_event_handler_set = true;
        }

        taskEXIT_CRITICAL(&_eventChangeMutex);

        return status;
    }

    // method to setup a queue to receive events from the GPIO interrupt, rather than an event loop
    void Gpio::setQueueHandle(xQueueHandle Gpio_e_q)
    {
        taskENTER_CRITICAL(&_eventChangeMutex);
        _clearEventHandlers();
        _interrupt_args._queue_handle = Gpio_e_q;
        _interrupt_args._queue_enabled = true;
        taskEXIT_CRITICAL(&_eventChangeMutex);
    }

    esp_err_t Gpio::_clearEventHandlers()
    {
        esp_err_t status {ESP_OK};

        if(_interrupt_args._custom_event_handler_set)
        {
            esp_event_handler_unregister_with(_interrupt_args._custom_event_loop_handle, INPUT_EVENTS, _interrupt_args._pin, _event_handle);
            _interrupt_args._custom_event_handler_set = false;
        }
        else if (_interrupt_args._event_handler_set)
        {
            esp_event_handler_instance_unregister(INPUT_EVENTS, _interrupt_args._pin, nullptr);
            _interrupt_args._event_handler_set = false;
        }

        _interrupt_args._queue_handle = nullptr;
        _interrupt_args._queue_enabled = false;

        return status;
    }

    // GPIO output methods
    esp_err_t Gpio::on(void)
    {
        _level = true;
        return gpio_set_level(_pin, _active_low ? 0 : 1);
    }

    esp_err_t Gpio::off(void)
    {
        _level = false;
        return gpio_set_level(_pin, _active_low ? 1 : 0);
    }

    esp_err_t Gpio::toggle(void)
    {
        _level = _level ? 0 : 1;
        return gpio_set_level(_pin, _level ? 1 : 0);
    }

    esp_err_t Gpio::setLevel(int level)
    {
        _level = _active_low ? !level : level;
        return gpio_set_level(_pin, _level);
    }
} // Namespace CPPGPIO
