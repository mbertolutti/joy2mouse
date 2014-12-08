#include "xbox360_controller.hpp"

namespace xbox360_controller {

std::string to_string(button button)
{
    switch (button)
    {
        case button::face_a: return "Face A";
        case button::face_b: return "Face B";
        case button::face_x: return "Face X";
        case button::face_y: return "Face Y";
        case button::left_bumper: return "Left Bumper";
        case button::right_bumper: return "Right Bumper";
        case button::back: return "Back";
        case button::start: return "Start";
        case button::guide: return "Guide";
        case button::left_stick: return "Left Stick";
        case button::right_stick: return "Right Stick";
    }
    return "Unknown";
}

std::string to_string(axis axis)
{
    switch (axis)
    {
        case axis::left_stick_x: return "Left Stick X";
        case axis::left_stick_y: return "Left Stick Y";
        case axis::left_trigger: return "Left Trigger";
        case axis::right_stick_x: return "Right Stick X";
        case axis::right_stick_y: return "Right Stick Y";
        case axis::right_trigger: return "Right Trigger";
        case axis::dpad_x: return "D-Pad X";
        case axis::dpad_y: return "D-Pad Y";
    }
    return "Unknown";
}

} // namespace xbox360_controller

