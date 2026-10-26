/* Host check of the actual switch logic (common/hook.c): kill switch, enabled, and the trial
 * boot taken by the first part that starts, but never on the boot that armed it.
 * Copyright (c) 2026 Anton Perk. CC BY-NC-SA 4.0, see LICENSE and NOTICE. */
#include "mcn_qnx.h"
#include "mcn_hook.h"

int puts(const char *);
const char mcn_component[] = "check";

#define CHECK(x)                            \
    do {                                    \
        if (!(x)) {                         \
            puts("hook check failed: " #x); \
            abort();                        \
        }                                   \
    } while (0)

/* A tiny file system: the paths that exist. */
static const char *files[8];
static long now_s;

static int find(const char *path)
{
    int i;

    for (i = 0; i < 8; i++) {
        if (files[i] && !strcmp(files[i], path)) {
            return i;
        }
    }
    return -1;
}

static void put(const char *path)
{
    int i;

    for (i = 0; i < 8 && files[i]; i++) {
    }
    CHECK(i < 8);
    files[i] = path;
}

int mcn_test_open(const char *path, int flags, ...)
{
    if (flags & O_CREAT) {
        if (find(path) < 0 && strcmp(path, MCN_LOG_FILE)) {
            put(path);
        }
        return 20;
    }
    return find(path) >= 0 ? 20 : -1;
}

int mcn_test_close(int fd)
{
    CHECK(fd == 20);
    return 0;
}

int mcn_test_unlink(const char *path)
{
    int i = find(path);

    if (i >= 0) {
        files[i] = NULL;
    }
    return i >= 0 ? 0 : -1;
}

int mcn_test_clock_gettime(int clock, struct timespec *ts)
{
    (void)clock;
    ts->tv_sec = now_s;
    ts->tv_nsec = 0;
    return 0;
}

static int boot(const char *a, const char *b)
{
    memset(files, 0, sizeof(files));
    if (a) {
        put(a);
    }
    if (b) {
        put(b);
    }
    now_s += 3; /* past the 2 s cache of mcn_active */
    return mcn_active();
}

int main(void)
{
    CHECK(boot(MCN_DIR "/trial", NULL));
    CHECK(find(MCN_DIR "/trial") < 0 && find(MCN_TRIAL_MARK) >= 0);
    CHECK(!boot(MCN_DIR "/trial", MCN_ARMED_MARK)); /* armed now: the next boot's trial */
    CHECK(find(MCN_DIR "/trial") >= 0);
    CHECK(!boot(MCN_DIR "/trial", "/media/mp003/mst2-cluster-nav-disable"));
    CHECK(find(MCN_DIR "/trial") >= 0); /* the kill switch does not use up the trial */
    CHECK(boot(MCN_TRIAL_MARK, NULL));
    CHECK(boot(MCN_DIR "/enabled", NULL));
    CHECK(boot(MCN_BOOT_MARK, NULL));
    CHECK(!boot(MCN_DIR "/enabled", "/media/mp009/mst2-cluster-nav-disable"));
    CHECK(!boot(MCN_DIR "/enabled", MCN_OFF_MARK)); /* runHMI.sh's guard kept the mod out */
    CHECK(!boot(MCN_DIR "/trial", MCN_OFF_MARK) && find(MCN_DIR "/trial") >= 0);
    CHECK(!boot(NULL, NULL));
    puts("hook: kill switch, guard, enabled, trial taken once and never on its arming boot: ok");
    return 0;
}
