#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <linux/joystick.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <X11/Xlib.h>
#include <X11/XF86keysym.h>

#include <boost/format.hpp>

#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>

#include "xbox360_controller.hpp"
#include "vec.hpp"

const char* joystick_interface = "/dev/input/js0";

constexpr unsigned cursor_update_hz = 60;

constexpr float cursor_speed = 25;
constexpr float cursor_gamma = 5.0f;
constexpr float cursor_accel_threshold = 0.25f;
constexpr float cursor_reset_threshold = 0.15f;
constexpr float max_cursor_accel = 2.0f;
constexpr float cursor_accel_time = 1.0f;
constexpr float cursor_accel_factor =
    cursor_accel_time * (max_cursor_accel - 1.0f) / cursor_update_hz;

constexpr float scroll_speed = 10;
constexpr float scroll_gamma = 3.0f;
constexpr float scroll_accel_threshold = 0.25f;
constexpr float scroll_reset_threshold = 0.15f;
constexpr float max_scroll_accel = 4.0f;
constexpr float scroll_accel_time = 2.0f;
constexpr float scroll_accel_factor =
    scroll_accel_time * (max_scroll_accel - 1.0f) / cursor_update_hz;

void send_button_event(Display* dpy, unsigned button, bool release)
{
    XEvent event = {};

    event.type = ButtonPress;
    event.xbutton.button = button;
    event.xbutton.same_screen = True;
    event.xbutton.subwindow = RootWindow(dpy, DefaultScreen(dpy));

    if (release)
    {
        event.type = ButtonRelease;
        event.xbutton.state = 0x100;
    }

    while (event.xbutton.subwindow)
    {
        event.xbutton.window = event.xbutton.subwindow;
        XQueryPointer(dpy, event.xbutton.window, &event.xbutton.root,
                      &event.xbutton.subwindow, &event.xbutton.x_root,
                      &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y,
                      &event.xbutton.state);
    }

    XSendEvent(dpy, PointerWindow, True, ButtonPressMask, &event);
    XFlush(dpy);
}

void send_keyboard_event(Display* dpy, unsigned key, bool release)
{
    XEvent event = {};

    Window focused_window;
    int revert;
    XGetInputFocus(dpy, &focused_window, &revert);

    event.xkey.display = dpy;
    event.xkey.window = focused_window;
    event.xkey.root = RootWindow(dpy, DefaultScreen(dpy));
    event.xkey.subwindow = None;
    event.xkey.time = CurrentTime;
    event.xkey.x = 1;
    event.xkey.y = 1;
    event.xkey.x_root = 1;
    event.xkey.y_root = 1;
    event.xkey.same_screen = True;
    event.xkey.keycode = XKeysymToKeycode(dpy, key);
    event.xkey.state = 0;

    event.type = release ? KeyRelease : KeyPress;

    XSendEvent(dpy, focused_window, True, KeyPressMask, &event);
    XFlush(dpy);
}

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
                           js_event ev, Display* dpy)
{
    switch (ev.type)
    {
        case JS_EVENT_BUTTON:
        {
            using xbox360_controller::button;

            auto updated_button = static_cast<button>(ev.number);
            bool pressed = ev.value ? true : false;

            // Left click
            if (updated_button == button::face_a)
            {
                send_button_event(dpy, Button1, !pressed);
            }

            // Right click
            else if (updated_button == button::face_b)
            {
                send_button_event(dpy, Button3, !pressed);
            }

            // Middle click
            else if (updated_button == button::face_x)
            {
                send_button_event(dpy, Button2, !pressed);
            }

            // Back
            else if (updated_button == button::left_bumper)
            {
                send_keyboard_event(dpy, XF86XK_Back, false);
                send_keyboard_event(dpy, XF86XK_Back, true);
            }

            // Forward
            else if (updated_button == button::right_bumper)
            {
                send_keyboard_event(dpy, XF86XK_Forward, false);
                send_keyboard_event(dpy, XF86XK_Forward, true);
            }

            const char* action = (pressed ? "pressed" : "released");
            std::cout << boost::str(boost::format("[%1%] was %2%\n")
                    % to_string(updated_button) % action);
        } break;
        case JS_EVENT_AXIS:
        {
            apply_axis_input(controller_state, ev);
        } break;
    }
}

int main()
{
    Display* dpy = XOpenDisplay(nullptr);
    if (dpy == None)
    {
        perror("error opening display");
        return EXIT_FAILURE;
    }

    int epfd = epoll_create(2);
    if (epfd < 0)
    {
        perror("error opening epoll interface");
        return EXIT_FAILURE;
    }

    int tfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (tfd < 0)
    {
        perror("error opening timer interface");
        return EXIT_FAILURE;
    }

    epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = tfd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, tfd, &event) < 0)
    {
        perror("error adding timer to epoll");
        return EXIT_FAILURE;
    }

    itimerspec ts = {};
    ts.it_interval.tv_nsec = 1000000000 / cursor_update_hz;
    ts.it_value = ts.it_interval;
    if (timerfd_settime(tfd, 0, &ts, nullptr) < 0)
    {
        perror("error setting timer");
        return EXIT_FAILURE;
    }

    int jsfd = open(joystick_interface, O_RDONLY);
    if (jsfd < 0)
    {
        perror("error opening joystick interface");
        return EXIT_FAILURE;
    }

    event.events = EPOLLIN;
    event.data.fd = jsfd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, jsfd, &event) < 0)
    {
        perror("error adding joystick to epoll");
        return EXIT_FAILURE;
    }

    xbox360_controller::input_state controller_state = {};

    float cursor_accel = 0.0f;
    float scroll_accel = 0.0f;
    float scroll_pos = 0.0f;

    for(;;)
    {
        int status = epoll_wait(epfd, &event, 1, -1);
        if (status < 1)
        {
            perror("epoll error");
            continue;
        }

        if (event.data.fd == jsfd)
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

            handle_joystick_event(controller_state, ev, dpy);
        }
        else if (event.data.fd == tfd)
        {
            std::uint64_t expirations;
            ssize_t bytes_read = read(tfd, &expirations, sizeof(expirations));
            if (bytes_read < 0)
            {
                perror("error reading timer event");
                break;
            }
            if (bytes_read != sizeof(expirations))
            {
                std::cerr << "error: short read from timer\n";
                continue;
            }

            controller_state.process_dead_zone();

            auto corrected = controller_state.corrected;
            auto left_stick = corrected.left_stick;
            auto right_stick = corrected.right_stick;

            auto left_magnitude = left_stick.length();
            if (left_magnitude)
            {
                left_magnitude = std::pow(left_magnitude, cursor_gamma);

                if (left_magnitude > cursor_accel_threshold)
                {
                    cursor_accel += left_magnitude * cursor_accel_factor;
                    cursor_accel = std::min(cursor_accel, max_cursor_accel);
                }
                else if (left_magnitude < cursor_reset_threshold)
                {
                    cursor_accel = 1.0f;
                }

                int dx = cursor_accel * cursor_speed * left_stick[0];
                int dy = cursor_accel * cursor_speed * left_stick[1];
                XWarpPointer(dpy, None, None, 0, 0, 0, 0, dx, dy);
            }
            else
            {
                cursor_accel = 1.0f;
            }

            auto right_magnitude = right_stick.length();
            if (right_magnitude)
            {
                right_magnitude = std::pow(right_magnitude, scroll_gamma);

                if (right_magnitude > scroll_accel_threshold)
                {
                    scroll_accel += right_magnitude * scroll_accel_factor;
                    scroll_accel = std::min(scroll_accel, max_scroll_accel);
                }
                else if (right_magnitude < scroll_reset_threshold)
                {
                    scroll_accel = 1.0f;
                }

                scroll_pos += scroll_accel * scroll_speed
                    * right_stick[1] / cursor_update_hz;

                // Scroll up
                while (scroll_pos <= -1.0f)
                {
                    send_button_event(dpy, Button4, true);
                    send_button_event(dpy, Button4, false);
                    scroll_pos += 1.0f;
                }

                // Scroll down
                while (scroll_pos >= 1.0f)
                {
                    send_button_event(dpy, Button5, true);
                    send_button_event(dpy, Button5, false);
                    scroll_pos -= 1.0f;
                }
            }
            else
            {
                scroll_accel = 1.0f;
                scroll_pos = 0.0f;
            }

            XSync(dpy, False);
        }
    }

    close(jsfd);
    close(epfd);

    XCloseDisplay(dpy);
}

