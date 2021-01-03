// This file is copied from https://github.com/davidcutting42/ArduinoTimers
// All Credit to David Cutting

#include <Arduino.h>

#if defined(ARDUINO_AVR_MEGA) || defined(ARDUINO_AVR_MEGA2560)

#include "ATMEGA2560/Timer.h"

Timer TimerA(1);
Timer TimerB(3);
Timer TimerC(4);
Timer TimerD(5);

ISR(TIMER1_OVF_vect)
{
    TimerA.isrCallback();
}

ISR(TIMER3_OVF_vect)
{
    TimerB.isrCallback();
}

ISR(TIMER4_OVF_vect)
{
    TimerC.isrCallback();
}

ISR(TIMER5_OVF_vect)
{
    TimerD.isrCallback();
}

#elif defined(ARDUINO_AVR_UNO)      // Todo: add other 328 boards for compatibility

#include "ATMEGA328/Timer.h"

Timer TimerA(1);
Timer TimerB(2);

ISR(TIMER1_OVF_vect)
{
    TimerA.isrCallback();
}

ISR(TIMER2_OVF_vect)
{
    TimerB.isrCallback();
}

#elif defined(ARDUINO_SAMD_ZERO)

#include "ATSAMD21/Timer.h"

Timer TimerA((TcCount16*)TC3);
Timer TimerB((TcCount16*)TC4);

int number_of_tc3_calls = 0;
int number_of_tc4_calls = 0;

void TC3_Handler()
{
    TcCount16* TC = (TcCount16*) TC3;
    // If this interrupt is due to the compare register matching the timer count
    // we toggle the LED.
    if (TC->INTFLAG.bit.MC0 == 1)
    {
        TC->INTFLAG.bit.MC0 = 1;

        number_of_tc3_calls++;
        TimerA.isrCallback();
    }
}

void TC4_Handler()
{
    TcCount16* TC = (TcCount16*) TC4;
    // If this interrupt is due to the compare register matching the timer count
    // we toggle the LED.
    if (TC->INTFLAG.bit.MC0 == 1)
    {
        TC->INTFLAG.bit.MC0 = 1;

        number_of_tc4_calls++;
        TimerB.isrCallback();
    }
}

#endif
