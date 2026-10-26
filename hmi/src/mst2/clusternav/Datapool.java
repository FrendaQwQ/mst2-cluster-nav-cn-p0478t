/*
 * mst2-cluster-nav - phone navigation on the MQB instrument cluster.
 * Copyright (c) 2026 Anton Perk. CC BY-NC-SA 4.0, see LICENSE and NOTICE.
 */
package mst2.clusternav;

import de.vw.mib.asl.api.navbap.ASLNavBAPAPI;
import de.vw.mib.asl.api.navbap.ASLNavBAPFactory;
import de.vw.mib.asl.framework.api.framework.ASLFrameworkFactory;
import de.vw.mib.asl.framework.api.framework.Services;
import de.vw.mib.asl.framework.internal.framework.GenericCollector;

/**
 * Writes guidance into the HMI datapool exactly where the built-in navigation puts it
 * (de.vw.mib.asl.internal.navigation.bap.HsmTargetBap); the stock NavigationASLDataAdapter
 * turns it into BAP NavSD for the cluster and NavInfoAdapter into MOST NavInfo.
 * All writes run on the ASL task thread, like the stock producer.
 */
final class Datapool {
    /* ASL property: route guidance state (0 off, 1 active, 2 calculating). */
    private static final int ROUTE_GUIDANCE_STATE = 732;
    /* ASL list: BAP ManeuverDescriptor (main element, direction, z-level, side streets). */
    private static final int MANEUVER_DESCRIPTOR = 751;
    /* Distance unit as HsmTargetBap passes it: let the HMI pick km or miles. */
    private static final int UNIT_AUTO = 255;

    static void publish(final Guidance g) {
        try {
            services().getASLThreadSwitch().enqueue(new Runnable() {
                public void run() {
                    write(g);
                }
            });
        } catch (Throwable t) {
            /* The callers' threads must survive an ASL that is not up (yet). */
            Activator.log("ASL thread not available: " + t);
        }
    }

    private static Services services() {
        return ASLFrameworkFactory.getASLFrameworkAPI().getServices();
    }

    private static void write(Guidance g) {
        Services services = services();
        ASLNavBAPAPI bap = ASLNavBAPFactory.getNavBAPApi();

        GenericCollector maneuver = new GenericCollector();
        maneuver.setIntItem(0, g.active ? g.mainElement : Guidance.NO_SYMBOL);
        maneuver.setIntItem(1, g.active ? g.direction : 0);
        maneuver.setIntItem(2, 0);
        maneuver.setStringItem(3, g.active ? g.sideStreets : "");
        services.getASLListManager().getGenericASLList(MANEUVER_DESCRIPTOR)
            .updateList(new GenericCollector[] { maneuver });

        if (g.active) {
            bap.updateBapDistanceToNextManeuver(g.distanceToManeuver, 0, g.showBargraph, g.bargraph);
            bap.updateBapTurnToInfo(g.street, "");
            bap.updateBapTimeToDestinationRTT(g.remainingSeconds);
            bap.updateBapDistanceToDestination(g.distanceToDestination, UNIT_AUTO, false);
        } else {
            bap.updateBapDistanceToNextManeuver(-1, 0, false, 0);
            bap.updateBapTurnToInfo("", "");
            bap.updateBapTimeToDestinationRTT(-1);
            bap.updateBapDistanceToDestination(-1, UNIT_AUTO, false);
        }
        bap.updateBapLaneGuidance(g.active ? g.lanes : Lanes.EMPTY, g.active && g.lanes.length > 0);
        /* Last, so every consumer sees complete data when guidance turns active. */
        services.getASLPropertyManager().valueChangedInteger(ROUTE_GUIDANCE_STATE, g.active ? 1 : 0);
    }

    private Datapool() {
    }
}
