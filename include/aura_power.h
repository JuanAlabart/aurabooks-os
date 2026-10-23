#ifndef AURA_POWER_H
#define AURA_POWER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int battery_percent; /* 0 to 100 */
    bool is_charging;
    char time_str[16];   /* "14:05" */
} aura_system_status_t;

/* Read current battery level and system clock */
aura_system_status_t aura_power_get_status(void);

/* Put device into low-power E-Ink sleep */
void aura_power_suspend(void);

#ifdef __cplusplus
}
#endif

#endif /* AURA_POWER_H */
