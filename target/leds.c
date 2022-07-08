/*
 * Lights.c: <<<UNTESTED>>
 * 
 * Dependencies: wiringPi must be installed.  
 * Instructions for download, install and use 
 * are located at http://wiringpi.com 
 *
 * Compile with:  gcc -o Lights Lights.c -lwiringPi
 *
 * All pin numbers are wiringPi numbers unless otherwise specified.
 * 
 */
#include <stdio.h>
// #include <wiringPi.h>
#include "leds.h"

static int Nred = 1;   //North Red pin.
static int Ngreen = 2; //North Green pin.
static int Ered = 3;   //East Red pin.
static int Egreen = 4; //East Green pin.

static int led_state = 0;

void leds_init(void)
{
    // wiringPiSetup();
    // pinMode(Nred, OUTPUT);
    // pinMode(Ered, OUTPUT);
}

int leds_turn_on(void)
{
    // digitalWrite(Nred, HIGH);
    // digitalWrite(Ered, HIGH);
    led_state = 1;
    printf("Led turned on\n");
    return 0;
}

int leds_turn_off(void)
{
    // digitalWrite(Nred, LOW);
    // digitalWrite(Ered, LOW);
    led_state = 0;
    printf("Led turned off\n");
    return 0;
}


int leds_state(void)
{
    return led_state;
    // return digitalRead(Nred);

}
// int main (void)
// {
//   int i; 
//   int Delaytime = 10000; //Delay time in milleseconds.

//   wiringPiSetup();
 
//   for(i=1;i<5;i++) pinMode(i, OUTPUT); // Set pins as output.

//   while(1)
//   {
//     digitalWrite(Nred, HIGH);
//     digitalWrite(Ngreen, LOW);
//     digitalWrite(Ered, LOW);
//     digitalWrite(Egreen, HIGH);
//     delay(delaytime);

//     digitalWrite(Nred, LOW);
//     digitalWrite(Ngreen, HIGH);
//     digitalWrite(Ered, HIGH);
//     digitalWrite(Egreen, LOW);
//     delay(delaytime);
//   }
//   return 0 ;
// }