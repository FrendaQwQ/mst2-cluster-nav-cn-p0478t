/*
 * mst2-cluster-nav - phone navigation on the MQB instrument cluster.
 * Copyright (c) 2026 Anton Perk. CC BY-NC-SA 4.0, see LICENSE and NOTICE.
 */
package mst2.clusternav;

/**
 * Demo guidance: a fixed route that walks through the maneuver kinds and turn angles,
 * so an installation can be checked in a parked car (and the arrow encoding in mQbEMU)
 * without a phone. Enabled by the "demo" file in the mod's data directory.
 */
final class Demo implements Runnable {
    private static final long STEP_MS = 1000;
    private static final int APPROACH_M = 300;
    private static final int STEP_M = 50;

    /* main element, direction, side-street directions, street */
    private static final Object[][] ROUTE = {
        { new Integer(Guidance.TURN), new Integer(192), new int[] { 64 }, "Right turn 192" },
        { new Integer(Guidance.TURN), new Integer(64), new int[] { 192 }, "Left turn 64" },
        /* A U-turn is a loop to one side: 64 or 192; 0 and 128 draw nothing (mQbEMU). */
        { new Integer(Guidance.UTURN), new Integer(64), new int[0], "U-turn" },
        { new Integer(Guidance.TURN), new Integer(224), new int[0], "Slight right 224" },
        { new Integer(Guidance.TURN), new Integer(32), new int[0], "Slight left 32" },
        { new Integer(Guidance.ROUNDABOUT_RIGHT), new Integer(192), new int[] { 0, 64, 128 },
            "Roundabout exit 192" },
        { new Integer(Guidance.FOLLOW_STREET), new Integer(0), new int[0], "Follow street" },
        { new Integer(Guidance.ARRIVED), new Integer(0), new int[0], "Destination" },
    };

    private volatile boolean running = true;

    void stop() {
        running = false;
    }

    public void run() {
        int routeLeft = ROUTE.length * APPROACH_M;
        try {
            while (running) {
                for (int i = 0; i < ROUTE.length && running; i++) {
                    for (int d = APPROACH_M; d >= 0 && running; d -= STEP_M) {
                        Datapool.publish(step(ROUTE[i], d, routeLeft));
                        routeLeft = Math.max(routeLeft - STEP_M, 0);
                        Thread.sleep(STEP_MS);
                    }
                }
                routeLeft = ROUTE.length * APPROACH_M;
            }
        } catch (InterruptedException e) {
            /* stopped */
        }
        Datapool.publish(Guidance.inactive());
    }

    private static Guidance step(Object[] row, int distance, int routeLeft) {
        Guidance g = new Guidance();
        g.active = true;
        g.mainElement = ((Integer) row[0]).intValue();
        g.direction = ((Integer) row[1]).intValue();
        int[] sides = (int[]) row[2];
        StringBuffer s = new StringBuffer();
        for (int i = 0; i < sides.length; i++) {
            s.append((char) sides[i]);
        }
        g.sideStreets = s.toString();
        g.street = (String) row[3];
        g.distanceToManeuver = distance;
        g.showBargraph = true;
        g.bargraph = distance * 100 / APPROACH_M;
        g.distanceToDestination = routeLeft;
        /* About 50 km/h. */
        g.remainingSeconds = routeLeft * 72L / 1000L;
        return g;
    }
}
