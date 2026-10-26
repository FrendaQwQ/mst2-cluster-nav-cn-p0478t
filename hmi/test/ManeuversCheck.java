/*
 * mst2-cluster-nav - host self-check of the maneuver mapping (JDK 8: hmi/check.sh).
 * Copyright (c) 2026 Anton Perk. CC BY-NC-SA 4.0, see LICENSE and NOTICE.
 */
package mst2.clusternav;

public final class ManeuversCheck {
    private static int failures;

    private static void expect(String what, int type, int angle, boolean lht, int main, int dir) {
        int[] got = Maneuvers.map(type, angle, lht);
        if (got[0] != main || got[1] != dir) {
            failures++;
            System.out.println("FAIL " + what + ": got " + got[0] + "/" + got[1]
                + ", want " + main + "/" + dir);
        }
    }

    public static void main(String[] args) {
        int none = Maneuvers.NO_ANGLE;
        expect("left turn", 1, -90, false, 13, 64);
        expect("right turn", 2, 90, false, 13, 192);
        expect("follow road", 5, none, false, 11, 0);
        expect("u-turn right-hand traffic, no angle", 4, none, false, 25, 64);
        expect("u-turn left-hand traffic, no angle", 4, none, true, 25, 192);
        expect("u-turn by angle sign", 4, 170, false, 25, 192);
        expect("u-turn unknown angle", 4, 1000, false, 25, 64);
        expect("roundabout exit -90", 30, -90, false, 21, 64);
        expect("roundabout exit +90", 30, 90, false, 21, 192);
        expect("roundabout exit 180", 31, 180, false, 21, 128);
        expect("roundabout exit -67 -> 48", 29, -67, false, 21, 48);
        expect("roundabout u-turn", 19, none, false, 21, 128);
        expect("roundabout exit 1, no angle", 28, none, false, 21, 192);
        expect("roundabout exit 2, angle 0", 29, 0, false, 21, 0);
        expect("roundabout exit 3, no angle", 30, none, false, 21, 64);
        expect("roundabout exit 1, left-hand traffic", 28, none, true, 22, 64);
        expect("exit roundabout", 7, none, false, 26, 192);
        expect("start route slight right", 11, 40, false, 13, 224);
        expect("start route sharp left", 11, -150, false, 13, 96);
        expect("off ramp by sign", 8, -30, false, 13, 32);
        expect("off ramp no angle, right traffic", 8, none, false, 13, 224);
        expect("arrive left", 24, none, false, 3, 64);
        expect("end of road left slight", 20, -40, false, 13, 32);
        expect("end of road right, wrong sign", 21, -40, false, 13, 192);
        expect("change highway left", 52, none, false, 12, 64);
        expect("unknown type", 99, none, false, 1, 0);
        String sides = Maneuvers.sideStreets(new int[] { -90, 0, 90, 1000 }, 90);
        if (sides.length() != 2 || sides.charAt(0) != 64 || sides.charAt(1) != 0) {
            failures++;
            System.out.println("FAIL side streets");
        }
        Guidance g = PhoneLink.parse("active=1\ntype=2\nangle=90\nroads=-90,0,90\ndist=100\n"
            + "step=400\nstreet=Седьмая улица\nrtt=600\ndest=2500\n");
        if (!g.active || g.mainElement != 13 || g.direction != 192 || g.bargraph != 25
            || !g.showBargraph || !g.street.equals("Седьмая улица") || g.remainingSeconds != 600
            || g.distanceToDestination != 2500 || g.sideStreets.length() != 2) {
            failures++;
            System.out.println("FAIL phone link parse: " + g);
        }
        if (PhoneLink.parse("active=0").active) {
            failures++;
            System.out.println("FAIL phone link end");
        }
        System.out.println(failures == 0 ? "maneuvers: ok" : "maneuvers: " + failures + " failure(s)");
        System.exit(failures == 0 ? 0 : 1);
    }
}
