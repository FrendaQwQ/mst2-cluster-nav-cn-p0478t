/*
 * mst2-cluster-nav - phone navigation on the MQB instrument cluster.
 * Copyright (c) 2026 Anton Perk. CC BY-NC-SA 4.0, see LICENSE and NOTICE.
 */
package mst2.clusternav;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;

/**
 * Guidance from SAL (CarPlay route guidance decoded from the stock iAP2 log; Android Auto
 * navigation status is planned): one UDP datagram on 127.0.0.1 per update, UTF-8 text, one
 * "key=value" per line:
 *
 *   active=1           0 ends guidance
 *   type=2             iAP2 maneuver type 0..53
 *   angle=90           exit angle, degrees, negative = left (absent: unknown)
 *   lht=0              1 for left-hand traffic
 *   roads=-90,0,90     junction road angles (side streets)
 *   dist=250           metres to the maneuver
 *   step=400           metres from the previous maneuver (for the approach bar)
 *   street=Main St     road the car is on (the app's, else the one the last maneuver led onto)
 *   rtt=900            seconds to the destination
 *   dest=12000         metres to the destination
 *
 * SAL also sends "video=1" about once a second while the phone's cluster stream connection is
 * open (frames or not: a still map sends none), and "video=0" when it closes; that datagram
 * carries nothing else. While either the video or the
 * guidance is alive the phone owns the cluster (ClusterCarPlay).
 *
 * Silence for GUIDANCE_TIMEOUT_MS also ends guidance, and for VIDEO_TIMEOUT_MS the video, so a
 * dead phone or process cannot leave the cluster to it.
 */
final class PhoneLink implements Runnable {
    static final int PORT = 39750;
    private static final int GUIDANCE_TIMEOUT_MS = 5000;
    private static final int VIDEO_TIMEOUT_MS = 3000;

    private volatile boolean running = true;
    private DatagramSocket socket;
    private boolean active, video;
    private long guidanceMs, videoMs;

    void stop() {
        running = false;
        if (socket != null) {
            socket.close();
        }
    }

    public void run() {
        try {
            socket = new DatagramSocket(PORT, InetAddress.getByName("127.0.0.1"));
            socket.setSoTimeout(1000);
            byte[] buf = new byte[4096];
            while (running) {
                DatagramPacket p = new DatagramPacket(buf, buf.length);
                try {
                    socket.receive(p);
                    received(new String(p.getData(), 0, p.getLength(), "UTF-8"));
                } catch (java.io.InterruptedIOException timeout) {
                    /* checked below */
                }
                long now = System.currentTimeMillis();
                if (active && now - guidanceMs > GUIDANCE_TIMEOUT_MS) {
                    Datapool.publish(Guidance.inactive());
                    active = false;
                }
                if (video && now - videoMs > VIDEO_TIMEOUT_MS) {
                    video = false;
                }
                ClusterCarPlay.setPhoneOwnsCluster(active || video);
                KombiMap.phoneVideo(video);
            }
        } catch (Throwable t) {
            if (running) {
                Activator.log("phone link stopped: " + t);
            }
        }
        if (active) {
            Datapool.publish(Guidance.inactive());
        }
        ClusterCarPlay.setPhoneOwnsCluster(false);
        KombiMap.phoneVideo(false);
    }

    private void received(String text) {
        if (text.startsWith("video=")) {
            video = text.startsWith("video=1");
            videoMs = System.currentTimeMillis();
            return;
        }
        Guidance g = parse(text);
        Datapool.publish(g);
        active = g.active;
        guidanceMs = System.currentTimeMillis();
    }

    static Guidance parse(String text) {
        Guidance g = new Guidance();
        int type = -1;
        int angle = Maneuvers.NO_ANGLE;
        boolean lht = false;
        int[] roads = null;
        int step = 0;
        int laneCount = 0;
        boolean laneCountSeen = false;
        String[] laneRows = new String[Lanes.MAX_LANES];
        boolean badLanes = false;
        int start = 0;
        while (start < text.length()) {
            int end = text.indexOf('\n', start);
            if (end < 0) {
                end = text.length();
            }
            String line = text.substring(start, end);
            start = end + 1;
            int eq = line.indexOf('=');
            if (eq <= 0) {
                continue;
            }
            String key = line.substring(0, eq);
            String value = line.substring(eq + 1);
            if (key.equals("active")) {
                g.active = value.equals("1");
            } else if (key.equals("type")) {
                type = number(value, -1);
            } else if (key.equals("angle")) {
                angle = number(value, Maneuvers.NO_ANGLE);
            } else if (key.equals("lht")) {
                lht = value.equals("1");
            } else if (key.equals("roads")) {
                roads = numbers(value);
            } else if (key.equals("dist")) {
                g.distanceToManeuver = number(value, -1);
            } else if (key.equals("step")) {
                step = number(value, 0);
            } else if (key.equals("street")) {
                g.street = value;
            } else if (key.equals("rtt")) {
                g.remainingSeconds = number(value, -1);
            } else if (key.equals("dest")) {
                g.distanceToDestination = number(value, -1);
            } else if (key.equals("lanes")) {
                badLanes |= laneCountSeen;
                laneCountSeen = true;
                laneCount = number(value, -1);
            } else if (key.startsWith("lane")) {
                int slot = number(key.substring(4), -1);
                if (slot < 0 || slot >= laneRows.length || laneRows[slot] != null) {
                    badLanes = true;
                } else {
                    laneRows[slot] = value;
                }
            }
        }
        int[] m = Maneuvers.map(type, angle, lht);
        g.mainElement = m[0];
        g.direction = m[1];
        g.sideStreets = Maneuvers.sideStreets(roads, angle);
        if (g.active && !badLanes) {
            g.lanes = Lanes.parse(laneRows, laneCount);
        }
        if (step > 0 && g.distanceToManeuver >= 0 && g.distanceToManeuver <= step) {
            g.showBargraph = true;
            g.bargraph = g.distanceToManeuver * 100 / step;
        }
        return g;
    }

    private static int number(String s, int fallback) {
        try {
            return Integer.parseInt(s.trim());
        } catch (NumberFormatException e) {
            return fallback;
        }
    }

    private static int[] numbers(String s) {
        int n = s.length() == 0 ? 0 : 1;
        for (int i = 0; i < s.length(); i++) {
            if (s.charAt(i) == ',') {
                n++;
            }
        }
        int[] out = new int[n];
        int start = 0;
        for (int i = 0; i < n; i++) {
            int end = s.indexOf(',', start);
            if (end < 0) {
                end = s.length();
            }
            out[i] = number(s.substring(start, end), 1000);
            start = end + 1;
        }
        return out;
    }
}
