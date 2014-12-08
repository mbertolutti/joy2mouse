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

enum class xbox360_controller_button
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

std::string to_string(xbox360_controller_button button)
{
    switch (button)
    {
        case xbox360_controller_button::face_a: return "Face A";
        case xbox360_controller_button::face_b: return "Face B";
        case xbox360_controller_button::face_x: return "Face X";
        case xbox360_controller_button::face_y: return "Face Y";
        case xbox360_controller_button::left_bumper: return "Left Bumper";
        case xbox360_controller_button::right_bumper: return "Right Bumper";
        case xbox360_controller_button::back: return "Back";
        case xbox360_controller_button::start: return "Start";
        case xbox360_controller_button::guide: return "Guide";
        case xbox360_controller_button::left_stick: return "Left Stick";
        case xbox360_controller_button::right_stick: return "Right Stick";
    }
    return "Unknown";
}

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
            auto button = static_cast<xbox360_controller_button>(ev.number);
            const char* action = (ev.value ? "pressed" : "released");
            std::cout << boost::str(boost::format("[%1%] was %2%\n")
                    % to_string(button) % action);
        } break;
        case JS_EVENT_AXIS:
        {
        } break;
        }
    }

    close(jsfd);
}
