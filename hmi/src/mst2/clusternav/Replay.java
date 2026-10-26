/*
 * mst2-cluster-nav - phone navigation on the MQB instrument cluster.
 * Copyright (c) 2026 Anton Perk. CC BY-NC-SA 4.0, see LICENSE and NOTICE.
 */
package mst2.clusternav;

import java.io.ByteArrayOutputStream;
import java.io.FileInputStream;
import java.io.InputStream;

/**
 * Plays a recorded route in a loop: the file holds PhoneLink messages separated by lines
 * "--- <milliseconds to wait before the next one>", and each goes through the same parsing and
 * maneuver mapping as live data from the hooks. Enabled by the "replay.txt" file in the mod's
 * data directory; used in mQbEMU and to show a real route in a parked car.
 */
final class Replay implements Runnable {
    private final String path;
    private volatile boolean running = true;

    Replay(String path) {
        this.path = path;
    }

    void stop() {
        running = false;
    }

    public void run() {
        try {
            String text = read(path);
            while (running) {
                int start = 0;
                while (running && start < text.length()) {
                    int sep = text.indexOf("\n---", start);
                    String message = sep < 0 ? text.substring(start) : text.substring(start, sep);
                    long wait = 1000;
                    if (sep < 0) {
                        start = text.length();
                    } else {
                        int eol = text.indexOf('\n', sep + 1);
                        if (eol < 0) {
                            eol = text.length();
                        }
                        String ms = text.substring(sep + 4, eol).trim();
                        if (ms.length() > 0) {
                            wait = Long.parseLong(ms);
                        }
                        start = eol + 1;
                    }
                    if (message.trim().length() > 0) {
                        Datapool.publish(PhoneLink.parse(message));
                    }
                    Thread.sleep(wait);
                }
            }
        } catch (Throwable t) {
            Activator.log("replay stopped: " + t);
        }
        Datapool.publish(Guidance.inactive());
    }

    static String read(String path) throws java.io.IOException {
        InputStream in = new FileInputStream(path);
        try {
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            byte[] buf = new byte[4096];
            for (int n; (n = in.read(buf)) > 0;) {
                out.write(buf, 0, n);
            }
            return new String(out.toByteArray(), "UTF-8");
        } finally {
            in.close();
        }
    }
}
