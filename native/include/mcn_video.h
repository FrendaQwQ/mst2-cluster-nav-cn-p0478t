/*
 * mst2-cluster-nav - phone navigation on the MQB instrument cluster.
 * Copyright (c) 2026 Anton Perk. CC BY-NC-SA 4.0, see LICENSE and NOTICE.
 *
 * Cluster video from SAL (writer: the phone's instrument-cluster stream) to DisplayManager
 * (reader: the cluster encoder's output) in shared memory /dev/shmem/mst2-cluster-nav-video.
 * Single writer, single reader; every slot is a sequence lock: the writer clears slot.seq,
 * fills the slot, then publishes the new sequence; the reader copies a slot and keeps it only
 * if slot.seq did not change meanwhile. Access units are H.264 Annex-B, SPS and PPS in front
 * of every IDR. The phone owns the cluster while its stream connection is open (alive_ms),
 * not only while frames come: a still map sends none, and the cluster keeps the last picture.
 * The reader sleeps on frame_ready, posted by the writer when the reader says it waits.
 */
#ifndef MCN_VIDEO_H
#define MCN_VIDEO_H

#define MCN_VIDEO_SHM "/mst2-cluster-nav-video"
#define MCN_VIDEO_MAGIC 0x4d434e56u /* "MCNV" */
#define MCN_VIDEO_VERSION 3u
/* About 0.8 s at 30 fps: DisplayManager waits for room in the MOST ring while the phone
 * refines a key frame with 30-80 KB P-frames; with 6 (0.2 s) it fell behind, skipped to the
 * next key frame and asked for one, which started the next burst. */
#define MCN_VIDEO_SLOTS 24u
#define MCN_VIDEO_SLOT_BYTES 256000u
/* The cluster's decoder takes frames up to 250000 bytes (fpke RIVP); larger ones are not sent. */
#define MCN_VIDEO_AU_MAX 250000u

#define MCN_VIDEO_IDR 1u

struct mcn_video_slot {
    volatile unsigned seq;
    unsigned size;
    unsigned flags;
    unsigned reserved;
    unsigned long long pts_us;
    unsigned char data[MCN_VIDEO_SLOT_BYTES];
};

struct mcn_video {
    unsigned magic;
    unsigned version;
    /* Sequence of the newest complete access unit; 0 = none since the stream started. */
    volatile unsigned write_seq;
    /* Changes whenever the phone's stream (re)starts; readers resynchronise on it. */
    volatile unsigned generation;
    /* Monotonic milliseconds of the last access unit; 0 while no stream is running. */
    volatile unsigned last_frame_ms;
    /* Set by the reader when it needs a key frame; cleared by the writer once it asked. */
    volatile unsigned want_idr;
    unsigned width;
    unsigned height;
    /* Monotonic ms of the writer's last pass while the phone's stream connection is open (a few
     * times a second, frames or not); 0 without a connection. */
    volatile unsigned alive_ms;
    /* The reader waits on frame_ready; the writer posts only then. */
    volatile unsigned waiting;
    /* Process-shared semaphore (QNX sem_t), valid when sem_ok; else the reader polls. */
    sem_t frame_ready;
    volatile unsigned sem_ok;
    unsigned reserved;
    struct mcn_video_slot slot[MCN_VIDEO_SLOTS];
};

#endif
