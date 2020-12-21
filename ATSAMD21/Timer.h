#ifndef ATSAMD21Timer_h
#define ATSAMD21Timer_h

#include "../VirtualTimer.h"
#include <Arduino.h>

#define CPU_HZ 48000000
#define TIMER_PRESCALER_DIV 1

class Timer : public VirtualTimer 
{
private:
    TcCount16* _TC = nullptr;
    int _timer_num;

    int _pwmPeriod;

    /// Timer resolution is 16-bits
    const unsigned long _timerResolution = 65536;
    unsigned long _lastMicroseconds = 0;

public:
    void (*isrCallback)();

    Timer(TcCount16* tc)
    : _TC(tc)
    {}

    void initialize()
    {
        // Commmon settings
        // Use the counter in "match-mode" (reset to zero at compare value)
        REG_GCLK_CLKCTRL = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID_TCC2_TC3) ;
        while ( GCLK->STATUS.bit.SYNCBUSY == 1 ); // wait for sync

        _TC->CTRLA.reg &= ~TC_CTRLA_ENABLE;
        while (_TC->STATUS.bit.SYNCBUSY == 1); // wait for sync

        // Use the 16-bit timer
        _TC->CTRLA.reg |= TC_CTRLA_MODE_COUNT16;
        while (_TC->STATUS.bit.SYNCBUSY == 1); // wait for sync

        // Use match mode so that the timer counter resets when the count matches the compare register
        _TC->CTRLA.reg |= TC_CTRLA_WAVEGEN_MFRQ;
        while (_TC->STATUS.bit.SYNCBUSY == 1); // wait for sync

        // Set prescaler to 1 
        // For a 48MHz clock and 16 counter, maximum duration ~1.3 ms
        _TC->CTRLA.reg |= TC_CTRLA_PRESCALER_DIV1;
        while (_TC->STATUS.bit.SYNCBUSY == 1); // wait for sync
    }

    void setPeriod(unsigned long microseconds)
    {
        // Do nothing if requested microseconds matches last setting
        if(microseconds == _lastMicroseconds)
        {
            return;
        }
        // Save the setting
        _lastMicroseconds = microseconds;

        // 
        int compareValue = (CPU_HZ / TIMER_PRESCALER_DIV / 1000000) * microseconds - 1;

        // Make sure the count is in a proportional position to where it was
        // to prevent any jitter or disconnect when changing the compare value.
        _TC->COUNT.reg = map(_TC->COUNT.reg, 0, _TC->CC[0].reg, 0, compareValue);
        _TC->CC[0].reg = compareValue;
        while (_TC->STATUS.bit.SYNCBUSY == 1);     // wait for sync    
    }

    void start()
    {
        // Enable the compare interrupt
        _TC->INTENSET.reg = 0;
        _TC->INTENSET.bit.MC0 = 1;

        NVIC_EnableIRQ(TC3_IRQn);

        _TC->CTRLA.reg |= TC_CTRLA_ENABLE;
        while (_TC->STATUS.bit.SYNCBUSY == 1); // wait for sync
    }

    void stop()
    {
        // todo
    }

    void attachInterrupt(void (*isr)())
    {
        isrCallback = isr;
	    
    }
    
    void detachInterrupt()
    {
       
    }
};

extern Timer TimerA;
extern Timer TimerB;



// void TC3_Handler() {
//   TcCount16* TC = (TcCount16*) TC3;
//   // If this interrupt is due to the compare register matching the timer count
//   // we toggle the LED.
//   if (TC->INTFLAG.bit.MC0 == 1) {
//     TC->INTFLAG.bit.MC0 = 1;
    
//     // Write callback here!!!
    
//     digitalWrite(LED_PIN, isLEDOn);
//     isLEDOn = !isLEDOn;
//   }
// }

#endif
