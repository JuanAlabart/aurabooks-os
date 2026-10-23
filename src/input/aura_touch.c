#include "aura_touch.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <linux/input.h>

#define SWIPE_THRESHOLD_PX   60
#define LONG_PRESS_MS        500

static uint64_t get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

aura_touch_t* aura_touch_init(const char *dev_path) {
    if (!dev_path) {
        dev_path = "/dev/input/event0";
    }

    int fd = open(dev_path, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        /* Try event1 if event0 fails */
        dev_path = "/dev/input/event1";
        fd = open(dev_path, O_RDONLY | O_NONBLOCK);
    }

    if (fd < 0) {
        fprintf(stderr, "[aura_touch] Warning: Cannot open touch device, touch input disabled.\n");
        return NULL;
    }

    aura_touch_t *touch = (aura_touch_t*)calloc(1, sizeof(aura_touch_t));
    if (!touch) {
        close(fd);
        return NULL;
    }

    touch->fd = fd;
    return touch;
}

void aura_touch_close(aura_touch_t *touch) {
    if (!touch) return;
    if (touch->fd >= 0) {
        close(touch->fd);
        touch->fd = -1;
    }
    free(touch);
}

bool aura_touch_poll(aura_touch_t *touch, aura_touch_event_t *out_event) {
    if (!touch || touch->fd < 0 || !out_event) return false;

    struct input_event ev[16];
    ssize_t bytes_read = read(touch->fd, ev, sizeof(ev));

    if (bytes_read <= 0) {
        /* Check for pending long-press */
        if (touch->is_pressed && !touch->long_press_fired) {
            uint64_t now = get_time_ms();
            if (now - touch->press_start_time_ms >= LONG_PRESS_MS) {
                touch->long_press_fired = true;
                out_event->type = AURA_TOUCH_LONG_PRESS;
                out_event->x = touch->current_x;
                out_event->y = touch->current_y;
                out_event->start_x = touch->press_start_x;
                out_event->start_y = touch->press_start_y;
                out_event->duration_ms = (uint32_t)(now - touch->press_start_time_ms);
                return true;
            }
        }
        return false;
    }

    size_t count = bytes_read / sizeof(struct input_event);
    bool touch_released = false;
    bool touch_moved = false;

    for (size_t i = 0; i < count; i++) {
        if (ev[i].type == EV_ABS) {
            /* Kindle Touch: ABS_MT_POSITION_X is usually code 0x35 (53) */
            if (ev[i].code == ABS_MT_POSITION_X || ev[i].code == ABS_X) {
                touch->current_x = ev[i].value;
                touch_moved = true;
            } else if (ev[i].code == ABS_MT_POSITION_Y || ev[i].code == ABS_Y) {
                touch->current_y = ev[i].value;
                touch_moved = true;
            }
        } else if (ev[i].type == EV_KEY) {
            if (ev[i].code == BTN_TOUCH) {
                if (ev[i].value == 1) {
                    /* Finger Down */
                    touch->is_pressed = true;
                    touch->press_start_x = touch->current_x;
                    touch->press_start_y = touch->current_y;
                    touch->press_start_time_ms = get_time_ms();
                    touch->long_press_fired = false;

                    out_event->type = AURA_TOUCH_DOWN;
                    out_event->x = touch->current_x;
                    out_event->y = touch->current_y;
                    out_event->start_x = touch->press_start_x;
                    out_event->start_y = touch->press_start_y;
                    out_event->duration_ms = 0;
                    return true;
                } else if (ev[i].value == 0) {
                    /* Finger Up */
                    touch->is_pressed = false;
                    touch_released = true;
                }
            }
        }
    }

    if (touch_released) {
        uint64_t now = get_time_ms();
        uint32_t duration = (uint32_t)(now - touch->press_start_time_ms);
        int dx = touch->current_x - touch->press_start_x;
        int dy = touch->current_y - touch->press_start_y;

        out_event->x = touch->current_x;
        out_event->y = touch->current_y;
        out_event->start_x = touch->press_start_x;
        out_event->start_y = touch->press_start_y;
        out_event->duration_ms = duration;

        /* Gesture detection: Swipes vs Taps */
        if (abs(dx) > SWIPE_THRESHOLD_PX && abs(dx) > abs(dy)) {
            out_event->type = (dx > 0) ? AURA_GESTURE_SWIPE_RIGHT : AURA_GESTURE_SWIPE_LEFT;
            return true;
        } else if (abs(dy) > SWIPE_THRESHOLD_PX && abs(dy) > abs(dx)) {
            out_event->type = (dy > 0) ? AURA_GESTURE_SWIPE_DOWN : AURA_GESTURE_SWIPE_UP;
            return true;
        } else {
            /* Clean Tap */
            out_event->type = AURA_TOUCH_UP;
            return true;
        }
    }

    return false;
}
