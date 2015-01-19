#ifndef JOY2MOUSE_XBOX360_CONTROLLER_HPP
#define JOY2MOUSE_XBOX360_CONTROLLER_HPP

#include <string>

#include "vec.hpp"

namespace xbox360_controller {

enum class button
{
    face_a,
    face_b,
    face_x,
    face_y,
    left_bumper,
    right_bumper,
    back,
    start,
    guide,
    left_stick,
    right_stick
};

enum class axis
{
    left_stick_x,
    left_stick_y,
    left_trigger,
    right_stick_x,
    right_stick_y,
    right_trigger,
    dpad_x,
    dpad_y
};

std::string to_string(button button);
std::string to_string(axis axis);

struct analog_state
{
    math::vec2f left_stick;
    math::vec2f right_stick;
    float left_trigger;
    float right_trigger;
};

struct input_state
{
    void process_dead_zone();

    analog_state uncorrected;
    analog_state corrected;
    math::vec2f dpad;

    bool left_stick_down;
    bool right_stick_down;
};

} // namespace xbox360_controller

#endif // !defined(JOY2MOUSE_XBOX360_CONTROLLER_HPP)

