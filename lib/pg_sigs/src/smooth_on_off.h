#ifndef _SMOOTH_ON_OFF_H
#define _SMOOTH_ON_OFF_H

#include <algorithm>
#include <functional>

class SmoothOnOff
{
public:
    SmoothOnOff(int pin, float transition_ms)
    : _pin_out(pin),
     _transition_ms(transition_ms)
    {}

    void begin(void)
    {
        _last_time_ms = millis();
        boundOutputLevel();
        pinMode(_pin_out, OUTPUT);
    }

    void setOnOffLevels(float on_level, float off_level)
    {
        _on_level = on_level;
        _off_level = off_level;
    }

    void flipPolarity(void)
    {
        float on_tmp = _on_level;
        _on_level = _off_level;
        _off_level = on_tmp;
    }

    void turnOn(void)
    {
        _is_on = true;
    }

    void turnOff(void)
    {
        _is_on = false;
    }

    void fullOn(void)
    {
        _is_on = true;
        _output = _on_level;
    }

    void fullOff(void)
    {
        _is_on = false;
        _output = _off_level;
    }

    void set(bool turn_on)
    {
        _is_on = turn_on;

    }

    void toggleOnOff()
    {
        _is_on = !_is_on;
    }

    void setLoopCallback(std::function<void(float)> cb)
    {
        _do_on_loop = cb;
    }

    void loop(void)
    {
        // Get current time and computed elapsed time
        unsigned long next_ms = millis();
        unsigned long elapsed_ms = next_ms - _last_time_ms;

        if(elapsed_ms > 0)
        {
            // Store millis for next call to loop
            _last_time_ms = next_ms;

            // Compute size of change to apply in a cycle
            float delta_out = (_on_level - _off_level) * static_cast<float>(elapsed_ms) / _transition_ms;
            // Direction of change is based on direction
            _output += (_is_on ? delta_out : -delta_out);
        }
        // Bound _output at the on and off levels
        boundOutputLevel();

        if(_do_on_loop)
        {
            _do_on_loop(_output);
        }

        // Set output
        analogWrite(_pin_out, static_cast<int>(_output));
    }

private:
    /// Ensure _current_level is bound to on/off levels
    void boundOutputLevel(void)
    {
        auto upper = std::max(_on_level, _off_level);
        auto lower = std::min(_on_level, _off_level);

        // clamp level between on and off
        _output = std::max(_output,lower);
        _output = std::min(_output,upper);
    }

    // State variables
    bool _is_on = false;
    float _output = 0.0f;
    unsigned long _last_time_ms = 0;

    // Parameters
    int _pin_out = -1;
    float _off_level = 0.0f;
    float _on_level = 255.0f;
    float _transition_ms = 750.0f;

    unsigned long _max_delta_ms = 100;

    std::function<void(float)> _do_on_loop;
};

#endif
