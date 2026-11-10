/*
 * mst2-cluster-nav - phone navigation on the MQB instrument cluster.
 * Copyright (c) 2026 Anton Perk. CC BY-NC-SA 4.0, see LICENSE and NOTICE.
 *
 * The CN P0478T/791D output canvas matches the 800x480 CarPlay picture of Patch1. The whole
 * picture fits in macroblocks, so its coded and visible sizes are equal. With zoom-out off,
 * stream.c forwards the phone's H.264. The two transcoded paths use the same output canvas:
 *   - Android Auto: keep the 1280x720 video mode and take the centered, aligned 800x480 area.
 *   - CarPlay zoom-out: offer a bigger screen, then scale its decoded picture down to 800x480.
 * The decode and encode run on the i.MX6 VPU, the zoom-out scale on the CPU (the GC320 through
 * Screen only point-samples, common/scale.c). mcn_geom() is the arithmetic that picks the request
 * size and the source rectangle; it is host-tested. The backend (sal/vpu.c) turns phone access
 * units into 800x480 ones, published through include/mcn_video.h exactly as stream.c does.
 */
#ifndef MCN_TRANSCODE_H
#define MCN_TRANSCODE_H

#ifdef MCN_HOST
#include <stdint.h>
#else
#include "mcn_qnx.h"
#endif

/* The visible 791D picture is already macroblock-aligned. */
#define MCN_TC_VIS_W   800
#define MCN_TC_VIS_H   480
#define MCN_TC_CODED_W MCN_TC_VIS_W
#define MCN_TC_CODED_H MCN_TC_VIS_H
#define MCN_TC_CROP_RIGHT  (MCN_TC_CODED_W - MCN_TC_VIS_W)
#define MCN_TC_CROP_BOTTOM (MCN_TC_CODED_H - MCN_TC_VIS_H)

enum mcn_tc_mode {
    MCN_TC_OFF = 0,        /* no transcode: CarPlay 1:1, stream.c forwards the phone's frames */
    MCN_TC_AA,             /* Android Auto: crop a 1:1 content area out of a 1280x720 frame */
    MCN_TC_CARPLAY_ZOOM    /* CarPlay zoom-out: scale an N-times bigger frame to the canvas */
};

/*
 * What to ask the phone for and how to turn its frame into 800x480. Pixel offsets and sizes
 * meant for the VPU are multiples of 16 across and 2 down (chroma); the advertised screen keeps
 * the canvas aspect apart from the required alignment.
 */
struct mcn_geom {
    enum mcn_tc_mode mode;
    int req_w, req_h;          /* alt screen / video size advertised to the phone */
    int margin_w, margin_h;    /* Android Auto width/height margin (0 for zoom) */
    int src_x, src_y;          /* top-left of the content rectangle in the decoded frame */
    int src_w, src_h;          /* its size: equal to the coded target, or scaled down to it */
    int scale;                 /* 1 if src must be scaled to the canvas, 0 if copied 1:1 */
};

/*
 * Fills g for the mode. factor_milli is the zoom ratio x1000 for MCN_TC_CARPLAY_ZOOM
 * (1000 = 1:1, 1500 = 1.5x, clamped to 1000..1900 for the i.MX6 VPU's ~1920 width). Returns 0
 * on success, -1 for an unknown mode.
 */
int mcn_geom(enum mcn_tc_mode mode, int factor_milli, struct mcn_geom *g);

/*
 * Appends the cluster's video and input Service entries for Android Auto to the
 * ServiceDiscoveryResponse (raw protobuf, mcn_aa_sdr.c), numbered video_id and input_id.
 * Returns the byte count, or 0 if g is not MCN_TC_AA or it does not fit.
 */
int mcn_aa_sdr(uint8_t *out, unsigned cap, int video_id, int input_id, const struct mcn_geom *g);

/*
 * The VPU transcoder (sal/vpu.c): decode the phone's H.264 and re-encode it for the 791D,
 * publishing into video. mcn_vpu_open returns the owner token of a fresh
 * transcoder (non-zero), or 0 when the VPU is not ready (the caller then publishes nothing: the
 * cluster stays stock, never the mod). Only the current token's holder feeds or closes; a stale
 * token (its transcoder already closed or replaced) is a no-op, so a receiver that ends late
 * never touches its successor's. The calls are serialised inside vpu.c.
 */
struct mcn_video;
unsigned mcn_vpu_open(const struct mcn_geom *geom, struct mcn_video *video);
void mcn_vpu_feed(unsigned token, const uint8_t *au, unsigned n);
void mcn_vpu_close(unsigned token);
/*
 * After a feed: 1 when the decoder lost its reference and the phone should send a key frame
 * (cleared by the call; the transcoder already waits for the next IDR), -1 when this transcoder
 * gave up or the token is stale (publish nothing, stop claiming the cluster), 0 otherwise.
 */
int mcn_vpu_poll(unsigned token);

/*
 * Android Auto cluster video (sal/gal_video.c): offer the cluster screen from the SAL discovery
 * hook (gal.c), drive focus and the transcoder from the SAL worker loop (iap2.c). Offered while
 * the mod is active and the cluster view is not "native arrows only".
 */
void mcn_gal_video_offer(void *router, void *response);
void mcn_gal_video_tick(void);
void mcn_gal_video_stop(void);

/*
 * Bilinear NV12 downscale (common/scale.c), the CarPlay zoom-out scale on the CPU: src_y/src_c
 * are the Y and interleaved-CbCr planes of one frame at sstride; it writes dw x dh into
 * dst_y/dst_c at dstride, one memcpy per row (dst may be uncached). sw a multiple of 4, all
 * dimensions even, dw < sw and dh < sh. work: MCN_SCALE_WORK(sw, dw) bytes of cached memory.
 */
#define MCN_SCALE_WORK(sw, dw) ((((sw) + 8 + 3) & ~3) + (dw))
void mcn_scale_nv12(const uint8_t *src_y, const uint8_t *src_c, int sstride, int sw, int sh,
                    uint8_t *dst_y, uint8_t *dst_c, int dstride, int dw, int dh, uint8_t *work);

#endif
