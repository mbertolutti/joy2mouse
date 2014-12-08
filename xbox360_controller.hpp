#ifndef JOY2MOUSE_XBOX360_CONTROLLER_HPP
#define JOY2MOUSE_XBOX360_CONTROLLER_HPP

#include <string>

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

} // namespace xbox360_controller

#endif // !defined(JOY2MOUSE_XBOX360_CONTROLLER_HPP)

