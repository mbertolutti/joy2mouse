#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <linux/joystick.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h>

#include <boost/format.hpp>

#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <chrono>

#include "xbox360_controller.hpp"
#include "vec.hpp"

const char* joystick_interface = "/dev/input/js0";

constexpr unsigned cursor_update_hz = 60;

constexpr float cursor_speed = 15;
constexpr float cursor_gamma = 2.5f;
constexpr float cursor_accel_threshold = 0.25f;
constexpr float cursor_reset_threshold = 0.2f;
constexpr float max_cursor_accel = 2.5f;
constexpr float cursor_accel_time = 2.0f;
constexpr float cursor_accel_factor =
    cursor_accel_time * (max_cursor_accel - 1.0f) / cursor_update_hz;

constexpr float scroll_speed = 7;
constexpr float scroll_gamma = 3.0f;
constexpr float scroll_accel_threshold = 0.25f;
constexpr float scroll_reset_threshold = 0.2f;
constexpr float max_scroll_accel = 4.0f;
constexpr float scroll_accel_time = 2.0f;
constexpr float scroll_accel_factor =
    scroll_accel_time * (max_scroll_accel - 1.0f) / cursor_update_hz;

constexpr float volume_up_speed = 7;
constexpr float volume_up_gamma = 3.0f;
constexpr float volume_up_accel_threshold = 0.25f;
constexpr float volume_up_reset_threshold = 0.2f;
constexpr float max_volume_up_accel = 4.0f;
constexpr float volume_up_accel_time = 2.0f;
constexpr float volume_up_accel_factor =
    volume_up_accel_time * (max_volume_up_accel - 1.0f) / cursor_update_hz;

constexpr float volume_down_speed = 7;
constexpr float volume_down_gamma = 3.0f;
constexpr float volume_down_accel_threshold = 0.25f;
constexpr float volume_down_reset_threshold = 0.2f;
constexpr float max_volume_down_accel = 4.0f;
constexpr float volume_down_accel_time = 2.0f;
constexpr float volume_down_accel_factor =
    volume_down_accel_time * (max_volume_down_accel - 1.0f) / cursor_update_hz;

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

void send_keyboard_event(Display* dpy, unsigned key, bool release, unsigned state = 0)
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
    event.xkey.state = state;

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
                send_keyboard_event(dpy, XF86XK_Back, !pressed);
            }

            // Forward
            else if (updated_button == button::right_bumper)
            {
                send_keyboard_event(dpy, XF86XK_Forward, !pressed);
            }

            // Ctrl + w
            else if (updated_button == button::face_y)
            {
                send_keyboard_event(dpy, XK_W, !pressed, ControlMask);
            }

            // mute
            else if (updated_button == button::guide)
            {
                send_keyboard_event(dpy, XF86XK_AudioMute, !pressed);
            }

            // return
            else if (updated_button == button::back)
            {
                send_keyboard_event(dpy, XK_Return, !pressed);
            }

            // space
            else if (updated_button == button::start)
            {
                send_keyboard_event(dpy, XK_space, !pressed);
            }

            // left
            else if (updated_button == button::start)
            {
                send_keyboard_event(dpy, XK_Left, !pressed);
            }

            // up
            else if (updated_button == button::start)
            {
                send_keyboard_event(dpy, XK_Up, !pressed);
            }

            // right
            else if (updated_button == button::start)
            {
                send_keyboard_event(dpy, XK_Right, !pressed);
            }

            // down
            else if (updated_button == button::start)
            {
                send_keyboard_event(dpy, XK_Down, !pressed);
            }

            // stick buttons
            else if (updated_button == button::left_stick)
            {
                controller_state.left_stick_down = pressed;
            }
            else if (updated_button == button::right_stick)
            {
                controller_state.right_stick_down = pressed;
            }

            if (controller_state.left_stick_down &&
                controller_state.right_stick_down)
            {
                send_keyboard_event(dpy, XK_Escape, false);
                send_keyboard_event(dpy, XK_Escape, true);
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
    math::vec2f cursor_accum = { 0.0f, 0.0f };
    float scroll_accel = 0.0f;
    float scroll_acum = 0.0f;
    float volume_down_accel = 0.0f;
    float volume_up_accel = 0.0f;
    float volume_acum = 0.0f;

    math::vec2f old_dpad = { 0.0f, 0.0f };

    using namespace std::chrono;
    using clock_type = steady_clock;

    auto key_repeat_time = milliseconds(250);
    auto key_repeat_interval = milliseconds(50);

    auto last_dpad_x = steady_clock::now();
    auto last_dpad_y = last_dpad_x;
    bool repeat_dpad_x = false;
    bool repeat_dpad_y = false;

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
            auto left_trigger = corrected.left_trigger;
            auto right_trigger = corrected.right_trigger;

            auto left_magnitude = left_stick.length();
            if (left_magnitude)
            {
                left_magnitude = std::pow(left_magnitude, cursor_gamma);
                left_stick = left_stick.normalized() * left_magnitude;

                if (left_magnitude > cursor_accel_threshold)
                {
                    cursor_accel += left_magnitude * cursor_accel_factor;
                    cursor_accel = std::min(cursor_accel, max_cursor_accel);
                }
                else if (left_magnitude < cursor_reset_threshold)
                {
                    cursor_accel = 1.0f;
                }

                cursor_accum += cursor_accel * cursor_speed * left_stick;

                if (std::abs(cursor_accum[0]) >= 1.0f ||
                    std::abs(cursor_accum[1]) >= 1.0f)
                {
                    int dx = cursor_accum[0];
                    int dy = cursor_accum[1];

                    XWarpPointer(dpy, None, None, 0, 0, 0, 0, dx, dy);
                    XSync(dpy, False);

                    cursor_accum[0] -= dx;
                    cursor_accum[1] -= dy;
                }
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

                scroll_acum += scroll_accel * scroll_speed
                    * right_stick[1] / cursor_update_hz;

                // Scroll up
                while (scroll_acum <= -1.0f)
                {
                    send_button_event(dpy, Button4, true);
                    send_button_event(dpy, Button4, false);
                    scroll_acum += 1.0f;
                }

                // Scroll down
                while (scroll_acum >= 1.0f)
                {
                    send_button_event(dpy, Button5, true);
                    send_button_event(dpy, Button5, false);
                    scroll_acum -= 1.0f;
                }
            }
            else
            {
                scroll_accel = 1.0f;
                scroll_acum = 0.0f;
            }

            if (left_trigger)
            {
                left_trigger = std::pow(left_trigger, volume_down_gamma);

                if (left_trigger > volume_down_accel_threshold)
                {
                    volume_down_accel += left_trigger * volume_down_accel_factor;
                    volume_down_accel = std::min(volume_down_accel, max_volume_down_accel);
                }
                else if (left_trigger < volume_down_reset_threshold)
                {
                    volume_down_accel = 1.0f;
                }

                volume_acum -= volume_down_accel * volume_down_speed
                    * left_trigger / cursor_update_hz;
            }
            else
            {
                volume_down_accel = 1.0f;
            }

            if (right_trigger)
            {
                right_trigger = std::pow(right_trigger, volume_up_gamma);

                if (right_trigger > volume_up_accel_threshold)
                {
                    volume_up_accel += right_trigger * volume_up_accel_factor;
                    volume_up_accel = std::min(volume_up_accel, max_volume_up_accel);
                }
                else if (right_trigger < volume_up_reset_threshold)
                {
                    volume_up_accel = 1.0f;
                }

                volume_acum += volume_up_accel * volume_up_speed
                    * right_trigger / cursor_update_hz;
            }
            else
            {
                volume_up_accel = 1.0f;
            }

            if (!left_trigger && !right_trigger)
            {
                volume_acum = 0;
            }

            // Volume down
            while (volume_acum <= -1.0f)
            {
                send_keyboard_event(dpy, XF86XK_AudioLowerVolume, false);
                send_keyboard_event(dpy, XF86XK_AudioLowerVolume, true);
                volume_acum += 1.0f;
            }

            // Volume up
            while (volume_acum >= 1.0f)
            {
                send_keyboard_event(dpy, XF86XK_AudioRaiseVolume, false);
                send_keyboard_event(dpy, XF86XK_AudioRaiseVolume, true);
                volume_acum -= 1.0f;
            }

            auto now = clock_type::now();
            auto dpad = controller_state.dpad;

            if (dpad[0] != old_dpad[0])
            {
                if (old_dpad[0] > 0.5)
                {
                    send_keyboard_event(dpy, XK_Right, true);
                }
                else if (old_dpad[0] < -0.5)
                {
                    send_keyboard_event(dpy, XK_Left, true);
                }

                if (dpad[0] > 0.5)
                {
                    send_keyboard_event(dpy, XK_Right, false);
                }
                else if (dpad[0] < -0.5)
                {
                    send_keyboard_event(dpy, XK_Left, false);
                }

                last_dpad_x = clock_type::now();
                repeat_dpad_x = false;
            }

            if (dpad[1] != old_dpad[1])
            {
                if (old_dpad[1] > 0.5)
                {
                    send_keyboard_event(dpy, XK_Down, true);
                }
                else if (old_dpad[1] < -0.5)
                {
                    send_keyboard_event(dpy, XK_Up, true);
                }

                if (dpad[1] > 0.5)
                {
                    send_keyboard_event(dpy, XK_Down, false);
                }
                else if (dpad[1] < -0.5)
                {
                    send_keyboard_event(dpy, XK_Up, false);
                }

                last_dpad_y = clock_type::now();
                repeat_dpad_y = false;
            }

            if (!repeat_dpad_x && now - last_dpad_x >= key_repeat_time)
            {
                repeat_dpad_x = true;
                last_dpad_x += key_repeat_time - key_repeat_interval;
            }

            if (repeat_dpad_x)
            {
                while (now - last_dpad_x >= key_repeat_interval)
                {
                    if (dpad[0] > 0.5)
                    {
                        send_keyboard_event(dpy, XK_Right, true);
                        send_keyboard_event(dpy, XK_Right, false);
                    }
                    else if (dpad[0] < -0.5)
                    {
                        send_keyboard_event(dpy, XK_Left, true);
                        send_keyboard_event(dpy, XK_Left, false);
                    }

                    last_dpad_x += key_repeat_interval;
                }
            }

            if (!repeat_dpad_y && now - last_dpad_y >= key_repeat_time)
            {
                repeat_dpad_y = true;
                last_dpad_y += key_repeat_time - key_repeat_interval;
            }

            if (repeat_dpad_y)
            {
                while (now - last_dpad_y >= key_repeat_interval)
                {
                    if (dpad[1] > 0.5)
                    {
                        send_keyboard_event(dpy, XK_Down, true);
                        send_keyboard_event(dpy, XK_Down, false);
                    }
                    else if (dpad[1] < -0.5)
                    {
                        send_keyboard_event(dpy, XK_Up, true);
                        send_keyboard_event(dpy, XK_Up, false);
                    }

                    last_dpad_y += key_repeat_interval;
                }
            }

            old_dpad = dpad;
        }
    }

    close(jsfd);
    close(epfd);

    XCloseDisplay(dpy);
}

