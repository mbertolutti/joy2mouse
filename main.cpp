#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <linux/joystick.h>

#include <boost/format.hpp>

#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>

const char* joystick_interface = "/dev/input/js0";

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

int main()
{
    int jsfd = open(joystick_interface, O_RDONLY);
    if (jsfd < 0)
    {
        perror("error opening joystick interface");
        return EXIT_FAILURE;
    }

    for(;;)
    {
        js_event ev;
        ssize_t bytes_read = read(jsfd, &ev, sizeof(ev));
        if (bytes_read < 0)
        {
            perror("error reading joystick event");
            break;
        }
        if (bytes_read != sizeof(ev))
        {
            std::cerr << "error: short read from joystick\n";
            continue;
        }

        switch (ev.type)
        {
        case JS_EVENT_BUTTON:
        {
            auto button = static_cast<xbox360_controller::button>(ev.number);
            const char* action = (ev.value ? "pressed" : "released");
            std::cout << boost::str(boost::format("[%1%] was %2%\n")
                    % to_string(button) % action);
        } break;
        case JS_EVENT_AXIS:
        {
            auto axis = static_cast<xbox360_controller::axis>(ev.number);
            double value = ev.value / 32767.0;
            std::cout << boost::str(boost::format("Axis %1%: %2%\n")
                    % to_string(axis) % value);
        } break;
        }
    }

    close(jsfd);
}
