// Author           : Filip Berka (s203163@student.pg.edu.pl)
// Created On       : 8.06.2026
// Last Modified On : 9.06.2026
// Version          : 1.0
//
// Description      :
// 	software for measuring energy consumption
// 	utilizes perf_event_open system call 
//
// Licensed under GPL (see /usr/share/common-licenses/GPL for more details
// or contact // the Free Software Foundation for a copy)
// 
// Generative AI statement (keep ONE line below, delete the others):
// * I did NOT use GenAI tools while developing this code.

#include <bits/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <stdint.h>

#define TYPE "/sys/bus/event_source/devices/power/type"
#define PKG "/sys/bus/event_source/devices/power/events/energy-pkg"
#define SCALE "/sys/bus/event_source/devices/power/events/energy-pkg.scale"

sig_atomic_t keepRunning = 1;


int get_type(const char *path, int *val) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    int res = fscanf(f, "%d", val);
    fclose(f);
    return (res == 1) ? 0 : -1;
}

int get_pkg(const char *path, uint64_t *val) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    char buf[64];
    if (!fgets(buf, sizeof(buf), f)) {
        fclose(f);
        return -1;
    }
    fclose(f);
    char *ptr = strchr(buf, '=');
    if (ptr) {
        *val = strtoull(ptr + 1, NULL, 16);
        return 0;
    }
    return -1;
}

int get_scale(const char *path, double *val) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    int res = fscanf(f, "%lf", val);
    fclose(f);
    return (res == 1) ? 0 : -1;
}

void end(){
    keepRunning = 0;
}

int main() {
    // setup the SIGINT handler
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = end;
    sigaction(SIGINT, &action, NULL);

    // read the required values and setup the perf_event_attr structure
    int type;
    uint64_t config;
    double scale;
    get_type(TYPE, &type);
    get_pkg(PKG, &config);
    get_scale(SCALE, &scale);
    struct perf_event_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.type = type;
    attr.size = sizeof(attr);
    attr.config = config;
    attr.disabled = 1;

    // setup the counter
    int fd = syscall(SYS_perf_event_open, &attr, -1, 0, -1, 0);
    if (fd == -1) {
        printf("Error calling perf_event_open\n");
        return 1;
    }
    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

    uint64_t val = 0, prevVal = 0;
    read(fd, &prevVal, sizeof(uint64_t));
    uint64_t startVal = prevVal;


    // main loop
    sleep(1);
    while (keepRunning) {
        if (read(fd, &val, sizeof(uint64_t)) == sizeof(uint64_t)) {
            double de = (val - prevVal) * scale;
            printf("Power consumption: %.2f W\n", de);
            prevVal = val;
        }
        sleep(1); 
    }

    // after SIGINT
    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    if (read(fd, &val, sizeof(uint64_t)) == sizeof(uint64_t)) {
        double total = (val - startVal) * scale;
        printf("\nTotal power used: %.2f J\n", total);
    }

    close(fd);
    return 0;
}
