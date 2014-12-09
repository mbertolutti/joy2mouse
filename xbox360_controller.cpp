#include <cmath>

#include "xbox360_controller.hpp"

namespace xbox360_controller {

void input_state::process_dead_zone()
{
    const float left_stick_dead_zone = 7849;
    const float right_stick_dead_zone = 8689;
    const float trigger_threshold = 30;

    float left_stick_magnitude = uncorrected.left_stick.length();
    if (left_stick_magnitude >= left_stick_dead_zone)
    {
        float magnitude = (left_stick_magnitude - left_stick_dead_zone)
            / (32767.0f - left_stick_dead_zone);
        corrected.left_stick = magnitude
            * uncorrected.left_stick.normalized();
    }
    else
    {
        corrected.left_stick = {0.0f, 0.0f};
    }

    float right_stick_magnitude = uncorrected.right_stick.length();
    if (right_stick_magnitude >= right_stick_dead_zone)
    {
        float magnitude = (right_stick_magnitude - right_stick_dead_zone)
            / (32767.0f - right_stick_dead_zone);
        corrected.right_stick = magnitude
            * uncorrected.right_stick.normalized();
    }
    else
    {
        corrected.right_stick = {0.0f, 0.0f};
    }

    float left_trigger_magnitude = std::abs(uncorrected.left_trigger);
    if (left_trigger_magnitude >= trigger_threshold)
    {
        float magnitude = (left_trigger_magnitude - trigger_threshold)
            / (32767.0f - trigger_threshold);
        corrected.left_trigger =
            std::copysign(magnitude, uncorrected.left_trigger);
    }
    else
    {
        corrected.left_trigger = 0.0f;
    }

    float right_trigger_magnitude = std::abs(uncorrected.right_trigger);
    if (right_trigger_magnitude >= trigger_threshold)
    {
        float magnitude = (right_trigger_magnitude - trigger_threshold)
            / (32767.0f - trigger_threshold);
        corrected.right_trigger =
            std::copysign(magnitude, uncorrected.right_trigger);
    }
    else
    {
        corrected.right_trigger = 0.0f;
    }
}

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

