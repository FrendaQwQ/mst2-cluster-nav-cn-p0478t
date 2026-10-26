#!/bin/ksh
# mst2-cluster-nav - settings: mcn_set.sh enable|disable|trial|arrows|map|auto|fps10|fps20|fps25|fps30|keyframe0|keyframe1|keyframe2|
#   scene-map|scene-app|display|display-reset|demo-on|demo-off|
#   status|diag
# Copyright (c) 2026 Anton Perk. CC BY-NC-SA 4.0, see LICENSE and NOTICE.
. /tsd/etc/persistence/esd/scripts/mcn_common.sh

if [ ! -e $MCN/mst2-cluster-nav.jar ]; then
  echo "mst2-cluster-nav is not installed."
  exit 1
fi
case "$1" in
  enable)
    if grep -q "$LIBDIR/" $CPU_CONF; then
      touch $MCN/enabled
      rm -f $MCN/off-reason
      echo "Loads on every boot."
    else
      echo "The mod took itself out of cpu.conf ($(cat $MCN/off-reason 2>/dev/null)). Use Install."
    fi
    ;;
  disable) rm -f $MCN/enabled $MCN/trial; echo "Not loaded from the next boot." ;;
  trial) touch /dev/shmem/mst2-cluster-nav-armed $MCN/trial; echo "The next boot loads the mod once." ;;
  arrows|map|auto) echo $1 > $MCN/mode; echo "Cluster mode: $1 (from the next CarPlay connection)" ;;
  fps10|fps20|fps25|fps30) echo ${1#fps} > $MCN/fps; echo "Cluster video: ${1#fps} fps (from the next CarPlay connection)" ;;
  # A frame cut short on the way to the cluster (MOST channel full) spoils the picture until the
  # next key frame; regular ones recover it sooner, but each starts a burst of refining frames.
  keyframe0) rm -f $MCN/keyframe; echo "Cluster video: a key frame only when needed (from the next CarPlay connection)" ;;
  keyframe1|keyframe2) echo ${1#keyframe} > $MCN/keyframe; echo "Cluster video: a key frame every ${1#keyframe} s (from the next CarPlay connection)" ;;
  scene-map) rm -f $MCN/scene; echo "Cluster: phone map only (from the next CarPlay connection)" ;;
  scene-app) echo app > $MCN/scene; echo "Cluster: phone map with its maneuver card, experimental (from the next CarPlay connection)" ;;
  display)
    # Calibration from the card: "width_mm height_mm safe_x safe_y safe_width safe_height".
    card=$(mcn_card) || { echo "Toolbox SD card not found."; exit 1; }
    if [ -s $card/mst2-cluster-nav-display.txt ]; then
      cp $card/mst2-cluster-nav-display.txt $MCN/display
      echo "Cluster geometry: $(cat $MCN/display) (from the next CarPlay connection)"
    else
      echo "Put mst2-cluster-nav-display.txt on the card, e.g. 244 91 377 56 257 250"
    fi
    ;;
  display-reset) rm -f $MCN/display; echo "Cluster geometry: default (244 91 377 56 257 250)" ;;
  demo-on) touch $MCN/demo; echo "Demo guidance on (takes effect after a reboot)." ;;
  demo-off) rm -f $MCN/demo; echo "Demo guidance off (takes effect after a reboot)." ;;
  status) ;;
  diag)
    # Run while CarPlay is connected: the mod's log, the packet filter, the iAP2 driver's log.
    card=$(mcn_card) || { echo "Toolbox SD card not found."; exit 1; }
    out=$card/mst2-cluster-nav-diag.txt
    {
      for c in "pfctl -s info" "pfctl -s rules" "pfctl -a port_rules -s rules" \
        "cat /tsd/var/networking/port_rules.conf" "ifconfig -a" "netstat -an" "ls -l /dev/shmem" \
        "cat /dev/shmem/mst2-cluster-nav.log" "cat /tsd/var/sal/carplay/iap2.cfg" \
        "cat /dev/shmem/mst2-cluster-nav-iap2.cfg" \
        "ls -l /dev/shmem/mst2-cluster-nav-rg-ready" \
        "ls -l $MCN" "cat $MCN/mode $MCN/fps $MCN/keyframe"; do
        echo "### $c"
        $c 2>&1
      done
    } > $out
    # The iAP2 driver's own log (mm-ipod -vvvvvv): every message, unknown ones in hex.
    cp /dev/shmem/iap2_carplay.log $card/mst2-cluster-nav-iap2.log 2>/dev/null
    echo "Saved $out"
    ;;
  *) echo "unknown setting $1"; exit 1 ;;
esac
echo "HMI $(mcn_hmi_version); registered: $(mcn_installed && echo yes || echo no)"
[ -e $MCN/off-reason ] && echo "turned itself off: $(cat $MCN/off-reason)"
for c in sal dm ipod; do
  [ -e $MCN/off-$c ] && echo "$c hooks off after repeated crashes (Install turns them on again)"
done
echo "permanent: $([ -e $MCN/enabled ] && echo yes || echo no);" \
  "trial armed: $([ -e $MCN/trial ] && echo yes || echo no);" \
  "mode: $(cat $MCN/mode 2>/dev/null); fps: $(cat $MCN/fps 2>/dev/null || echo 25); key frame: $(cat $MCN/keyframe 2>/dev/null | sed "s/.*/every & s/" || true)$( [ -e $MCN/keyframe ] || echo "when needed");" \
  "scene: $([ -e $MCN/scene ] && echo map+card || echo map);" \
  "geometry: $(cat $MCN/display 2>/dev/null || echo default); demo: $([ -e $MCN/demo ] && echo on || echo off)"
