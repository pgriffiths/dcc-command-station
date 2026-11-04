#ifndef _PERIODIC_TRIGGER_H
#define _PERIODIC_TRIGGER_H

class PeriodicTrigger
{
public:
    PeriodicTrigger(float period_ms)
    : _period_ms(period_ms)
    {}

    void begin(void)
    {
        reset();
    }

    void reset(void)
    {
        _last_time_ms = millis();
    }

    void setPeriod(unsigned long period_ms)
    {
        _period_ms = period_ms;
    }

    bool loop(void)
    {
        // Get current time and computed elapsed time
        unsigned long next_ms = millis();
        unsigned long elapsed_ms = next_ms - _last_time_ms;

        // Return true on expiration
        if(elapsed_ms > _period_ms)
        {
            // Store millis to restart period
            _last_time_ms = next_ms;
            return true;
        }
        return false;
    }

private:
    // State variables
    unsigned long _last_time_ms = 0;

    // Parameters
    unsigned long _period_ms = 0.0f;
};

#endif
