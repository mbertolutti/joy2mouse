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

#include "xbox360_controller.hpp"
#include "vec.hpp"

const char* joystick_interface = "/dev/input/js0";

void apply_axis_input(xbox360_controller::input_state& controller_state,
                      js_event ev)
{
    using xbox360_controller::axis;

    auto& uncorrected = controller_state.uncorrected;
    auto value = ev.value;

    auto updated_axis = static_cast<axis>(ev.number);
    switch (updated_axis)
    {
        case axis::left_stick_x:
            uncorrected.left_stick[0] = value;
            break;

        case axis::left_stick_y:
            uncorrected.left_stick[1] = value;
            break;

        case axis::left_trigger:
            uncorrected.left_trigger = value;
            break;

        case axis::right_stick_x:
            uncorrected.right_stick[0] = value;
            break;

        case axis::right_stick_y:
            uncorrected.right_stick[1] = value;
            break;

        case axis::right_trigger:
            uncorrected.right_trigger = value;
            break;

        case axis::dpad_x:
            controller_state.dpad[0] = value / 32767.0f;
            break;

        case axis::dpad_y:
            controller_state.dpad[1] = value / 32767.0f;
            break;
    }
}

void handle_joystick_event(xbox360_controller::input_state& controller_state,
                           js_event ev)
{
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
            apply_axis_input(controller_state, ev);
        } break;
    }
}

int main()
{
    int jsfd = open(joystick_interface, O_RDONLY);
    if (jsfd < 0)
    {
        perror("error opening joystick interface");
        return EXIT_FAILURE;
    }

    xbox360_controller::input_state controller_state = {};

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

        handle_joystick_event(controller_state, ev);
    }

    close(jsfd);
}

