#include "aura_power.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static int read_int_file(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return -1;
    int val = -1;
    if (fscanf(fp, "%d", &val) != 1) {
        val = -1;
    }
    fclose(fp);
    return val;
}

aura_system_status_t aura_power_get_status(void) {
    aura_system_status_t status;
    status.battery_percent = 100;
    status.is_charging = false;
    status.time_str[0] = '\0';

    /* 1. Read battery percentage from Kindle sysfs nodes */
    const char *bat_paths[] = {
        "/sys/devices/system/yoshi_battery/yoshi_battery0/battery_capacity",
        "/sys/class/power_supply/mc13892_bat/capacity",
        "/sys/class/power_supply/battery/capacity",
        NULL
    };

    for (int i = 0; bat_paths[i] != NULL; i++) {
        int cap = read_int_file(bat_paths[i]);
        if (cap >= 0 && cap <= 100) {
            status.battery_percent = cap;
            break;
        }
    }

    /* 2. Check charging state */
    FILE *fp_status = fopen("/sys/devices/system/yoshi_battery/yoshi_battery0/battery_status", "r");
    if (!fp_status) {
        fp_status = fopen("/sys/class/power_supply/mc13892_bat/status", "r");
    }
    if (fp_status) {
        char buf[32];
        if (fgets(buf, sizeof(buf), fp_status)) {
            if (strstr(buf, "Charging") || strstr(buf, "Full")) {
                status.is_charging = true;
            }
        }
        fclose(fp_status);
    }

    /* 3. Read system time */
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    if (local) {
        snprintf(status.time_str, sizeof(status.time_str), "%02d:%02d", local->tm_hour, local->tm_min);
    } else {
        snprintf(status.time_str, sizeof(status.time_str), "12:00");
    }

    return status;
}

void aura_power_suspend(void) {
    printf("[aura_power] Entering low-power sleep state...\n");
    FILE *fp = fopen("/sys/power/state", "w");
    if (fp) {
        fputs("mem\n", fp);
        fclose(fp);
    }
}
