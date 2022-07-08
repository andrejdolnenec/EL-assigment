#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include "button.h"
#include "leds.h"
#include "comm.h"
// #include <wiringPi.h>

static int button_pin;

static pthread_t button_led_thread;

static bool is_button_pressed(void)
{
    // if(digitalRead(button_pin))
    // {
    //     //return the value
    // }
    return false;
}

/**
 * @brief Thread polling for the button press
 * 
 * @param arg 
 * @return void* 
 */
static void *button_read_thread (void * arg)
{
    while(1)
    {
        // Check Button pressed
        if(is_button_pressed())
        {
            if(leds_state() == 1)
            {
                leds_turn_off();
                comm_send_led_status("led_off");
            }
            else
            {
                leds_turn_on();
                comm_send_led_status("led_on");
            }
        }
        usleep(5000);

    }
}

int button_init(void)
{
    // pinMode(button_pin, INPUT);
    // Init stuff
    if (pthread_create(&button_led_thread, NULL, button_read_thread, NULL) != 0)
    {
        perror ("pthread_create(button_read_thread)");
        return -1;
    }
}