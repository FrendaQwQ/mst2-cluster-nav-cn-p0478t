#!/bin/ksh
# mst2-cluster-nav - install, or update, and arm one trial boot.
# Copyright (c) 2026 Anton Perk. CC BY-NC-SA 4.0, see LICENSE and NOTICE.
. /tsd/etc/persistence/esd/scripts/mcn_common.sh

echo "mst2-cluster-nav: install"
mcn_check_version || exit 1
CARD=$(mcn_card) || { echo "Aborted: no SD card with custom/mst2-cluster-nav."; exit 1; }
SRC=$CARD/custom/mst2-cluster-nav
mcn_check_unit $SRC/preload.list || exit 1

# Outside our own block, which names the patch's jar in its guard.
if mcn_strip_block $HMI_SH | grep -q 'NavActiveIgnore.jar'; then
  echo "Aborted: the toolbox 'dual navigation' patch (NavActiveIgnore.jar) is installed."
  echo "It replaces stock HMI classes and conflicts with this mod. Disable it first:"
  echo "Customization -> Navigation -> Disable dual navigation, then reboot."
  exit 1
fi

mkdir -p $MCN/backup
mcn_put $SRC/mst2-cluster-nav.jar $MCN/mst2-cluster-nav.jar 644 || { echo "Aborted: copy failed."; exit 1; }
mcn_mount_rw
mkdir -p $LIBDIR
for lib in $(awk '!/^#/ && NF == 2 { print $2 }' $SRC/preload.list); do
  if ! mcn_put $SRC/$lib $LIBDIR/$lib 755; then
    mcn_mount_ro
    echo "Aborted: $lib could not be copied intact; cpu.conf is unchanged."
    exit 1
  fi
done
mcn_mount_ro
[ -e $MCN/mode ] || echo auto > $MCN/mode

if ! mcn_installed; then
  # Stock copies before the first change: on the unit and, so that they survive a unit whose
  # HMI no longer starts, on the toolbox card. Nothing is changed unless all three exist.
  [ -e $MCN/backup/runHMI.sh ] || cp -p $HMI_SH $MCN/backup/runHMI.sh
  [ -e $MCN/backup/bundles.properties ] || cp -p $BUNDLES $MCN/backup/bundles.properties
  [ -e $MCN/backup/cpu.conf ] || cp -p $CPU_CONF $MCN/backup/cpu.conf
  for f in runHMI.sh bundles.properties cpu.conf; do
    if [ ! -s $MCN/backup/$f ]; then
      echo "Aborted: could not back up $f; nothing was changed."
      exit 1
    fi
  done
  mkdir -p $CARD/mst2-cluster-nav-backup &&
    cp -p $MCN/backup/runHMI.sh $MCN/backup/bundles.properties $MCN/backup/cpu.conf \
      $CARD/mst2-cluster-nav-backup/ &&
    echo "Stock files backed up to $CARD/mst2-cluster-nav-backup/ (keep this copy)."
fi
mcn_mount_rw
# The jar goes to the END of the boot class path, after the stock line that defines it.
mcn_rewrite $HMI_SH mcn_hmi_with_block $SRC/runHMI.block
mcn_rewrite $BUNDLES mcn_bundles_with_block $SRC/bundles.block
mcn_rewrite $CPU_CONF mcn_conf_with_preload $SRC/preload.list
mcn_mount_ro
if ! mcn_installed || ! grep -q "Lateload = .*,$BUNDLE_ID" $BUNDLES ||
    ! grep -q "$LIBDIR/" $CPU_CONF; then
  echo "Registration failed; restoring the stock files."
  mcn_mount_rw
  mcn_rewrite $HMI_SH cat $MCN/backup/runHMI.sh
  mcn_rewrite $BUNDLES cat $MCN/backup/bundles.properties
  mcn_rewrite $CPU_CONF cat $MCN/backup/cpu.conf
  mcn_mount_ro
  exit 1
fi
rm -f $MCN/off-reason
for c in sal dm ipod; do
  rm -f $MCN/off-$c $MCN/strikes-$c $MCN/busy-$c
done

rm -f $MCN/enabled
# The trial is the next boot's: the running hooks must not take it (native/include/mcn_hook.h).
touch /dev/shmem/mst2-cluster-nav-armed $MCN/trial
echo "Installed. The NEXT boot loads the mod once (trial)."
echo "If the unit works, choose 'Enable permanently'. Emergency: an SD card"
echo "with an empty file 'mst2-cluster-nav-disable' stops the mod before the HMI starts."
