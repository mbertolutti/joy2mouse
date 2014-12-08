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

const char* joystick_interface = "/dev/input/js0";

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

