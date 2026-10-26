/*
 * mst2-cluster-nav - phone lane guidance through the stock NavBAP API.
 * Copyright (c) 2026 Anton Perk. CC BY-NC-SA 4.0, see LICENSE and NOTICE.
 */
package mst2.clusternav;

import de.vw.mib.asl.api.navbap.INavLaneGuidanceDataNavBap;

/**
 * PhoneLink lane snapshot: lanes=N, lane0=index,status,highlight,angle,... .
 * Angles are signed iAP2 degrees; 1000 means absent/unknown. Positions are
 * sorted into road order before publishing. A malformed batch clears all lanes.
 * BAP enums are from P0480T LaneGuidance_LaneGuidance (javap): guidance 0/1/2,
 * unspecific type 0, no description 15, unspecified drawn borders 0.
 */
final class Lanes implements INavLaneGuidanceDataNavBap {
    static final INavLaneGuidanceDataNavBap[] EMPTY = new INavLaneGuidanceDataNavBap[0];
    static final int MAX_LANES = 16;
    static final int MAX_ANGLES = 16;
    private final int position;
    private final short direction;
    private final byte recommendation;
    private final byte[] sides;

    private Lanes(int index, int status, int highlight, int[] angles) {
        position = index;
        int primary = direction(highlight);
        int[] choices = new int[MAX_ANGLES + 1];
        int count = 0;
        for (int i = 0; i < angles.length; i++) {
            int code = direction(angles[i]);
            if (code != 255 && !contains(choices, count, code)) {
                choices[count++] = code;
            }
        }
        boolean unambiguous = primary != 255 || count == 1;
        if (primary == 255 && count > 0) {
            primary = choices[0];
        }
        direction = (short)primary;
        /* Never guess which exit to recommend when a multi-exit lane has no
         * valid highlight. Its other directions are still shown as side exits.
         */
        recommendation = (byte)(unambiguous && primary != 255 && status >= 0 && status <= 2
            ? status : 0);
        int n = 0;
        for (int i = 0; i < count; i++) {
            if (choices[i] != primary) {
                n++;
            }
        }
        sides = new byte[n];
        n = 0;
        for (int i = 0; i < count; i++) {
            if (choices[i] != primary) {
                sides[n++] = (byte)choices[i];
            }
        }
    }

    static INavLaneGuidanceDataNavBap[] parse(String[] rows, int count) {
        if (count <= 0 || count > MAX_LANES) {
            return EMPTY;
        }
        for (int i = count; i < rows.length; i++) {
            if (rows[i] != null) {
                return EMPTY;
            }
        }
        Lanes[] lanes = new Lanes[count];
        boolean drawable = false;
        for (int i = 0; i < count; i++) {
            int[] values = numbers(rows[i]);
            if (values == null || values.length < 3 || values.length > 3 + MAX_ANGLES
                || values[0] < 0 || values[0] >= 65535) {
                return EMPTY;
            }
            int[] angles = new int[values.length - 3];
            System.arraycopy(values, 3, angles, 0, angles.length);
            Lanes lane = new Lanes(values[0], values[1], values[2], angles);
            int at = i;
            while (at > 0 && lanes[at - 1].position > lane.position) {
                lanes[at] = lanes[at - 1];
                at--;
            }
            if ((at > 0 && lanes[at - 1].position == lane.position)
                || (at < i && lanes[at].position == lane.position)) {
                return EMPTY;
            }
            lanes[at] = lane;
            drawable |= lane.direction != 255;
        }
        return drawable ? lanes : EMPTY;
    }

    /* Same 16-sector BAP geometry as maneuver side streets: 0 ahead, 64 left,
     * 128 back, 192 right. 255 is explicitly invalid in stock LaneGuidance.
     */
    static int direction(int degrees) {
        if (degrees < -180 || degrees > 180) {
            return 255;
        }
        int sector = (Math.abs(degrees) * 16 + 180) / 360;
        return ((degrees < 0 ? sector : -sector) * 16) & 255;
    }

    private static boolean contains(int[] values, int count, int value) {
        for (int i = 0; i < count; i++) {
            if (values[i] == value) {
                return true;
            }
        }
        return false;
    }

    private static int[] numbers(String row) {
        if (row == null) {
            return null;
        }
        int count = 1;
        for (int i = 0; i < row.length(); i++) {
            if (row.charAt(i) == ',') {
                count++;
            }
        }
        if (count > 3 + MAX_ANGLES) {
            return null;
        }
        int[] result = new int[count];
        int start = 0;
        try {
            for (int i = 0; i < count; i++) {
                int end = row.indexOf(',', start);
                if (end < 0) {
                    end = row.length();
                }
                result[i] = Integer.parseInt(row.substring(start, end).trim());
                start = end + 1;
            }
            return result;
        } catch (NumberFormatException e) {
            return null;
        }
    }

    public short getLaneDirection() { return direction; }
    public byte getGuidanceInfo() { return recommendation; }
    public byte getLaneDescription() { return 15; }
    public byte getLaneMarkingLeft() { return 0; }
    public byte getLaneMarkingRight() { return 0; }
    public short getLaneType() { return 0; }
    public byte[] getLaneSideStreets() { return sides; }
}
