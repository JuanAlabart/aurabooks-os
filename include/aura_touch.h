#ifndef AURA_TOUCH_H
#define AURA_TOUCH_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AURA_TOUCH_NONE = 0,
    AURA_TOUCH_DOWN,
    AURA_TOUCH_MOVE,
    AURA_TOUCH_UP,        /* Simple tap */
    AURA_TOUCH_LONG_PRESS,
    AURA_GESTURE_SWIPE_LEFT,
    AURA_GESTURE_SWIPE_RIGHT,
    AURA_GESTURE_SWIPE_UP,
    AURA_GESTURE_SWIPE_DOWN
} aura_touch_type_t;

typedef struct {
    aura_touch_type_t type;
    int x;
    int y;
    int start_x;
    int start_y;
    uint32_t duration_ms;
} aura_touch_event_t;

typedef struct {
    int fd;
    bool is_pressed;
    int current_x;
    int current_y;
    int press_start_x;
    int press_start_y;
    uint64_t press_start_time_ms;
    bool long_press_fired;
    int max_raw_x;
    int max_raw_y;
} aura_touch_t;

/* Lifecycle */
aura_touch_t* aura_touch_init(const char *dev_path);
void          aura_touch_close(aura_touch_t *touch);

/* Non-blocking poll for touch events with autocalibrated coordinates */
bool aura_touch_poll(aura_touch_t *touch, aura_touch_event_t *out_event);

#ifdef __cplusplus
}
#endif

#endif /* AURA_TOUCH_H */
