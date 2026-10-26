/*
 * mst2-cluster-nav - phone navigation on the MQB instrument cluster.
 * Copyright (c) 2026 Anton Perk. CC BY-NC-SA 4.0, see LICENSE and NOTICE.
 *
 * Shared plumbing of the pass-through hooks (include/mcn_hook.h): finding the real function and
 * logging to the service's stdout, which the unit's launcher logs under the service name, and
 * the datagram link to the HMI bundle, and the switch that turns every hook into a pass-through.
 */
#include "mcn_qnx.h"
#include "mcn_hook.h"

/* Also kept in RAM for the GEM "Save diagnostics" (MCN_LOG_FILE), up to MCN_LOG_MAX bytes. */
static void log_line(const char *a, const char *b)
{
    char line[256];
    int n, fd;

    write(1, "[mst2-cluster-nav] ", 19);
    write(1, a, strlen(a));
    write(1, b, strlen(b));
    write(1, "\n", 1);
    n = snprintf(line, sizeof(line), "%u %s: %s%s\n", mcn_now_ms(), mcn_component, a, b);
    if (n <= 0) {
        return;
    }
    if (n >= (int)sizeof(line)) {
        n = sizeof(line) - 1;
        line[n - 1] = '\n';
    }
    fd = open(MCN_LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd >= 0) {
        if (lseek(fd, 0, SEEK_END) < MCN_LOG_MAX) {
            write(fd, line, (size_t)n);
        }
        close(fd);
    }
}

void mcn_log(const char *msg)
{
    log_line(msg, "");
}

void mcn_log2(const char *a, const char *b)
{
    log_line(a, b);
}

void *mcn_hook_enter(struct mcn_hook *hook)
{
    if (!hook->real) {
        void *lib = dlopen(hook->lib, RTLD_LAZY | RTLD_GLOBAL);
        hook->real = lib ? dlsym(lib, hook->name) : NULL;
        if (!hook->real) {
            mcn_log2("cannot resolve the real ", hook->name);
            abort();
        }
    }
    if (hook->calls++ < 3) {
        mcn_log2("call ", hook->name);
    }
    return hook->real;
}

void mcn_link_send(const char *text)
{
    static int fd = -1;
    struct sockaddr_in to;

    if (fd < 0) {
        fd = socket(AF_INET, SOCK_DGRAM, 0);
    }
    memset(&to, 0, sizeof(to));
    to.sin_len = sizeof(to);
    to.sin_family = AF_INET;
    to.sin_port = mcn_htons(MCN_LINK_PORT);
    to.sin_addr = 0x0100007fu; /* 127.0.0.1, network order on little-endian ARM */
    if (fd >= 0) {
        sendto(fd, text, strlen(text), 0, &to, sizeof(to));
    }
}

static int exists(const char *path)
{
    int fd = open(path, O_RDONLY);

    if (fd < 0) {
        return 0;
    }
    close(fd);
    return 1;
}

#ifndef O_TRUNC
#define O_TRUNC 0x0200 /* QNX <fcntl.h> */
#endif
#define CLOCK_WALL 0       /* CLOCK_REALTIME */
#define CRASH_WINDOW_S 180 /* a busy mark younger than this at start: the last run died busy */
#define STRIKES_OFF 2

static int component_off, busy_on;
static unsigned busy_ms;

static long wall_s(void)
{
    struct timespec t;

    clock_gettime(CLOCK_WALL, &t);
    return t.tv_sec;
}

/* MCN_DIR/<what>-<component>: one decimal number, 0 when missing. */
static void state_path(char *buf, unsigned n, const char *what)
{
    snprintf(buf, n, "%s/%s-%s", MCN_DIR, what, mcn_component);
}

static long state_read(const char *what)
{
    char path[96], text[16] = { 0 };
    long v = 0;
    int fd, i;

    state_path(path, sizeof(path), what);
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        return 0;
    }
    read(fd, text, sizeof(text) - 1);
    close(fd);
    for (i = 0; text[i] >= '0' && text[i] <= '9'; i++) {
        v = v * 10 + (text[i] - '0');
    }
    return v;
}

static void state_write(const char *what, long v)
{
    char path[96], text[16];
    int fd, n;

    state_path(path, sizeof(path), what);
    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd >= 0) {
        n = snprintf(text, sizeof(text), "%ld\n", v);
        write(fd, text, (size_t)n);
        close(fd);
    }
}

void mcn_guard_init(void)
{
    long busy, age, strikes;

    if (state_read("off")) {
        component_off = 1;
        mcn_log("off after repeated crashes; GEM Install turns it on again");
        return;
    }
    busy = state_read("busy");
    age = wall_s() - busy;
    if (busy && age >= 0 && age < CRASH_WINDOW_S) {
        /* The service died while the mod was working and the unit came straight back. */
        strikes = state_read("strikes") + 1;
        state_write("strikes", strikes);
        mcn_log("the last run ended while busy (crash suspected)");
        if (strikes >= STRIKES_OFF) {
            state_write("off", 1);
            component_off = 1;
            mcn_log("off after repeated crashes; GEM Install turns it on again");
        }
    }
    state_write("busy", 0);
}

void mcn_busy(int on)
{
    if (on) {
        busy_on = 1;
        busy_ms = mcn_now_ms();
        state_write("busy", wall_s());
    } else if (busy_on) {
        /* A clean end: earlier suspicions were not ours. */
        busy_on = 0;
        state_write("busy", 0);
        state_write("strikes", 0);
    }
}

void mcn_busy_tick(void)
{
    if (busy_on && mcn_now_ms() - busy_ms >= 60000) {
        busy_ms = mcn_now_ms();
        state_write("busy", wall_s());
    }
}

/* MCN_DIR/trial turned into MCN_TRIAL_MARK by the first part that sees it (mcn_hook.h). */
static int trial_boot(void)
{
    int fd;

    if (exists(MCN_TRIAL_MARK)) {
        return 1;
    }
    if (!exists(MCN_DIR "/trial") || exists(MCN_ARMED_MARK)) {
        return 0;
    }
    fd = open(MCN_TRIAL_MARK, O_WRONLY | O_CREAT, 0666);
    if (fd >= 0) {
        close(fd);
        unlink(MCN_DIR "/trial");
        mcn_log("trial boot: the trial is taken");
    }
    return 1;
}

int mcn_active(void)
{
    static unsigned checked_ms;
    static int active, said = -1;
    unsigned now = mcn_now_ms();

    if (component_off) {
        return 0;
    }
    if (said < 0 || now - checked_ms >= 2000) {
        checked_ms = now;
        char kill[] = "/media/mp000/mst2-cluster-nav-disable";
        int i;

        active = 1;
        for (i = 0; i < 10 && active; i++) {
            kill[11] = (char)('0' + i); /* the card mounter's /media/mp000 .. mp009 */
            active = !exists(kill);
        }
        /* The kill switch and the guard first: neither may use up the trial. */
        active = active && !exists(MCN_OFF_MARK) &&
                 (exists(MCN_DIR "/enabled") || exists(MCN_BOOT_MARK) || trial_boot());
        if (active != said) {
            said = active;
            mcn_log(active ? "active" : "off: disabled or SD card kill switch, stock behaviour");
        }
    }
    return active;
}
