/*
 * mst2-cluster-nav - phone navigation on the MQB instrument cluster.
 * Copyright (c) 2026 Anton Perk. CC BY-NC-SA 4.0, see LICENSE and NOTICE.
 *
 * H.264 avcC to Annex-B (avc.c).
 */
#ifndef MCN_AVC_H
#define MCN_AVC_H

#ifdef MCN_HOST
#include <stdint.h>
#else
#include "mcn_qnx.h"
#endif

struct avc {
    unsigned len_size; /* bytes of the NAL length prefix; 0 until a configuration arrived */
    unsigned ps_len;
    uint8_t ps[512]; /* SPS and PPS, Annex-B */
};

/* Takes an avcC record; returns 0 (configuration unchanged) if it is malformed. */
int avc_config(struct avc *a, const uint8_t *p, unsigned n);

/*
 * Writes the access unit `in` as Annex-B to `out`, SPS and PPS first if it is an IDR without
 * them. Returns the length, or 0 if the unit is malformed or does not fit in `cap`.
 */
unsigned avc_annexb(const struct avc *a, const uint8_t *in, unsigned n, uint8_t *out,
                    unsigned cap, int *idr);

#endif
