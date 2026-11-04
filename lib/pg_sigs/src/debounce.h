#ifndef _DEBOUNCE_H_
#define _DEBOUNCE_H_

#include <algorithm>
#include <functional>

class DebounceOnOff
{
public:
    enum Transition
    {
        None, On, Off
    };

    DebounceOnOff(uint32_t transition_ms)
    : _transition_ms(transition_ms)
    {}

    void begin(void)
    {
        _last_time_ms = millis();
        _debounce_timer_ms = _transition_ms;
        _is_on = false;
    }

    void setLoopCallback(std::function<void(bool)> cb)
    {
        _do_on_loop = cb;
    }

    void setChangeCallback(std::function<void(bool)> cb)
    {
        _do_on_change = cb;
    }

    Transition loop(bool input)
    {
        Transition on_off = None;
        unsigned long next_ms = millis();
        signed long elapsed_ms = next_ms - _last_time_ms;

        if(elapsed_ms > 0)
        {
            // Store millis for next call to loop
            _last_time_ms = next_ms;
        }

        if(_is_on == input)
        {
            // Output matches input, reset the timer
            _debounce_timer_ms = _transition_ms;
        }
        else
        {
            if(_debounce_timer_ms > 0)
            {
                _debounce_timer_ms -= elapsed_ms;
                // lower bound at zero
                _debounce_timer_ms = (_debounce_timer_ms < 0 ? 0 : _debounce_timer_ms);
            }
            else
            {
                // Declare a transition
                _is_on = input;
                on_off = (_is_on ? On : Off);

                // Call callback if defined
                if(_do_on_change)
                {
                    //_do_on_change(_is_on);
                }
            }
        }

        if(_do_on_loop)
        {
            //_do_on_loop(_is_on);
        }
        return on_off;
    }

private:
    // states
    bool _is_on = false;
    int32_t _debounce_timer_ms = 0;
    uint32_t _last_time_ms = 0;

    // parameters
    uint32_t _transition_ms = 0;

    std::function<void(int32_t)> _do_on_loop;
    std::function<void(int32_t)> _do_on_change;
};

#endif
