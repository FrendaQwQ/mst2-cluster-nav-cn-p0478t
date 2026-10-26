/*
 * CarPlay route guidance, independent of the QNX transport.
 * Copyright (c) 2026 Anton Perk. CC BY-NC-SA 4.0, see LICENSE and NOTICE.
 * Wire schema: the public iAP2 route-guidance description; P0480T ABI evidence: mQbEMU
 * research notes (phone-route-guidance.md, sections 8-11).
 */
#ifndef MCN_ROUTE_H
#define MCN_ROUTE_H

#ifdef MCN_HOST
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#else
#include "mcn_qnx.h"
#endif

#define MCN_RG_COMPONENT   0x10
#define MCN_RG_CACHE       6
#define MCN_RG_ROADS       16
#define MCN_RG_LANES       16
#define MCN_RG_LANE_ANGLES 16
#define MCN_RG_PACKET      8192
#define MCN_RG_SNAPSHOT    4096
#define MCN_RG_TEXT        256
#define MCN_RG_EMPTY_MS    3000
/* The phone sends route guidance only when it changes: Yandex Maps, parked, every ~6 s (car,
 * 2026-10-26). Silence is no end of guidance; only the arrival notice times out. */
#define MCN_RG_ARRIVED_MS  5000
#define MCN_RG_REIDENT     0x10000

struct mcn_maneuver {
    int valid, index, type, angle, lht, step;
    int roads[MCN_RG_ROADS], road_count;
    char street[MCN_RG_TEXT];
    char description[MCN_RG_TEXT];
};

struct mcn_lane {
    int index, status, highlight, angle_count;
    int angles[MCN_RG_LANE_ANGLES];
};

struct mcn_lane_info {
    int valid, index, count;
    struct mcn_lane lanes[MCN_RG_LANES];
};

/*
 * A maneuver counts as passed when the next one replaces it this close (metres to the maneuver
 * in the last status before): Yandex Maps switches at 10 m, at times 50 (car, 2026-10-26).
 */
#define MCN_RG_PASSED_M 50

struct mcn_route {
    struct mcn_maneuver maneuvers[MCN_RG_CACHE];
    struct mcn_lane_info lanes[MCN_RG_CACHE];
    unsigned next, lane_next, updated_ms, empty_ms;
    int seen, active, state, current, empty, distance, remaining, destination;
    int lane_index, show_lanes;
    /* The app named a road after a maneuver in this route (0x5202 field 4). */
    int after_roads;
    char road[MCN_RG_TEXT]; /* the app's current road (0x5201 field 3) */
    /* The road the last passed maneuver led onto: where the car is when the app names none. */
    char passed[MCN_RG_TEXT];
    int rerouted; /* state 5 since the current maneuver changed */
};

/* Complete, validated control messages only; invalid TLVs leave the state untouched. */
void mcn_route_reset(struct mcn_route *r);
int mcn_route_message(struct mcn_route *r, const uint8_t *msg, unsigned n,
                      unsigned now);
/* Guidance is on: a route is active and, once arrived, for MCN_RG_ARRIVED_MS more. */
int mcn_route_owns(const struct mcn_route *r, unsigned now);
int mcn_route_text(const struct mcn_route *r, unsigned now, char *out,
                   unsigned size);

/* Byte-stream reader of the driver's ASCII hex payload, including split reads/lines. */
struct mcn_route_log {
    uint8_t packet[MCN_RG_PACKET];
    unsigned used, want;
    int high;
    unsigned digits;
    unsigned start_match;
};
void mcn_route_log_reset(struct mcn_route_log *l);
int mcn_route_log_feed(struct mcn_route_log *l, struct mcn_route *r,
                       const char *text, unsigned n, unsigned now);
unsigned mcn_route_component(uint8_t *out);
unsigned mcn_route_start(uint8_t *out);
/* Rewrites only ident sendmsgs/recvmsgs, preserving pre-existing IDs and other sections. */
int mcn_route_config(const char *in, unsigned len, char *out, unsigned size);

/* iap2_ctrl_msg_table row (indexed by the message ID's high byte) and its msg_info
 * entries (indexed by the low byte); 12 bytes each on the 32-bit QNX ABI (E23, E24). */
struct mcn_msg_info {
    const char *name;
    uint16_t flags, reply;
    const void *params;
};
struct mcn_msg_group {
    const char *name;
    struct mcn_msg_info *info;
    uint32_t count;
};
/* Declares the received 0x52xx messages in an empty group 0x52 row; -1 if it is not empty. */
int mcn_route_msg_group(struct mcn_msg_group *group);

#endif
