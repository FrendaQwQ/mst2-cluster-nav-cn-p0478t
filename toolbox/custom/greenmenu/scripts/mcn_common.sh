#!/bin/ksh
# mst2-cluster-nav - shared definitions for the Green Engineering Menu scripts.
# Copyright (c) 2026 Anton Perk. CC BY-NC-SA 4.0, see LICENSE and NOTICE.

MCN=/tsd/var/mst2-cluster-nav
# The libraries LD_PRELOAD names live on the system partition, next to cpu.conf: a service whose
# preload is missing does not start (DisplayManager: the unit resets), so they must not depend on
# /tsd/var. They change only together with cpu.conf, by this installer or a firmware update.
LIBDIR=/tsd/lib/mst2-cluster-nav
HMI_SH=/tsd/hmi/runHMI.sh
BUNDLES=/tsd/hmi/HMI/config/bundles.properties
CPU_CONF=/tsd/etc/system/cpu.conf
BUNDLE_ID=58
# HMI builds this mod was verified on; datapool ids differ between builds.
HMI_VERSIONS="H29.319.93"

# The toolbox card that holds custom/mst2-cluster-nav.
mcn_card() {
  for v in /media/mp00?; do
    if [ -d $v/custom/mst2-cluster-nav ]; then
      echo $v
      return 0
    fi
  done
  return 1
}

mcn_hmi_version() {
  sed -n 's/^VERSION=//p' $HMI_SH | head -1
}

mcn_check_version() {
  v=$(mcn_hmi_version)
  for ok in $HMI_VERSIONS; do
    [ "$v" = "$ok" ] && return 0
  done
  if [ -e $MCN/force-version ]; then
    echo "HMI $v is not verified; continuing because $MCN/force-version exists."
    return 0
  fi
  echo "Aborted: HMI version $v is not verified for this mod (verified: $HMI_VERSIONS)."
  return 1
}

# The mod hooks services of the navigation build of MIB2 Standard (SAL runs CarPlay there,
# DisplayManager encodes the cluster map). The radio build (Bolero) has the same HMI version but
# no SAL: CarPlay runs elsewhere and nothing draws a cluster map. $1 is the card's preload.list.
mcn_check_unit() {
  if [ ! -x /tsd/bin/sal/tsd.mibstd2.sal.main ]; then
    echo "Aborted: this unit has no /tsd/bin/sal/tsd.mibstd2.sal.main. The mod needs the navigation"
    echo "build of MIB2 Standard (e.g. Amundsen); radio units (Bolero) are not supported."
    return 1
  fi
  for svc in $(awk '!/^#/ && NF == 2 && $1 !~ /:/ { print $1 }' "$1"); do
    if ! grep -q "^service $svc\$" $CPU_CONF; then
      echo "Aborted: $CPU_CONF has no service $svc; this unit is not supported."
      return 1
    fi
  done
  return 0
}

mcn_installed() {
  grep -q '>>> mst2-cluster-nav' $HMI_SH
}

# The toolbox's own helpers remount the system partition (/dev/hd0t177).
mcn_mount_rw() {
  . /tsd/etc/persistence/esd/scripts/util_mount.sh
}

mcn_mount_ro() {
  . /tsd/etc/persistence/esd/scripts/util_mount_ro.sh
}

# Append $MCN/<lib> to LD_PRELOAD of each service in preload.list ($1), or give the service an
# LD_PRELOAD line when it has none. Writes the result to stdout.
mcn_preload_conf() {
  awk -v list="$1" -v dir="$LIBDIR" '
    BEGIN { while ((getline l < list) > 0) if (l !~ /^#/ && split(l, f) == 2) lib[f[1]] = dir "/" f[2] }
    function flush() { if (svc != "" && (svc in lib) && !done) print "   envar LD_PRELOAD " lib[svc] }
    /^service / { flush(); svc = $2; done = 0; print; next }
    (svc in lib) && $1 == "envar" && $2 == "LD_PRELOAD" && !done { print $0 ":" lib[svc]; done = 1; next }
    { print }
    END { flush() }' ${2:-$CPU_CONF}
}

# Remove our libraries from every LD_PRELOAD in cpu.conf (any earlier location too). To stdout.
mcn_unpreload_conf() {
  sed -e '/^   envar LD_PRELOAD \/tsd\/[a-z]*\/mst2-cluster-nav\/[^:]*$/d' \
      -e 's#:/tsd/[a-z]*/mst2-cluster-nav/[^: ]*##g' $CPU_CONF
}

# Copy $1 to $2 under a new name, check it byte for byte, then rename: running services keep
# the old file mapped, and a bad copy never takes the name.
mcn_put() {
  cp $1 $2.new && cmp -s $1 $2.new && chmod $3 $2.new && mv -f $2.new $2
  rc=$?
  rm -f $2.new
  return $rc
}

# Our marked block removed from a file (the stock content around it is untouched), to stdout.
# The markers start their lines; the block's own guard quotes them inside a command.
mcn_strip_block() {
  sed '/^# >>> mst2-cluster-nav/,/^# <<< mst2-cluster-nav/d' "$1"
}

# bundles.properties without our bundle id and block.
mcn_bundles_remove() {
  mcn_strip_block $BUNDLES | sed 's/^\(Bundle\.Startup\.Lateload = .*\),'$BUNDLE_ID'$/\1/'
}

# The three files with the mod in, built from their content without it, so installing again
# (an update) replaces our parts instead of adding them twice. All write to stdout.
mcn_hmi_with_block() {
  mcn_strip_block $HMI_SH | sed '/^BOOTCLASSPATH="\$IFSDIR\/tsd.mibstd2.hmi.v2.jxe:/r '"$1"
}

mcn_bundles_with_block() {
  mcn_bundles_remove | sed 's/^\(Bundle\.Startup\.Lateload = .*\)$/\1,'$BUNDLE_ID'/' && cat "$1"
}

mcn_conf_with_preload() {
  mcn_unpreload_conf > $MCN/cpu.conf.mcn && mcn_preload_conf "$1" $MCN/cpu.conf.mcn
  rc=$?
  rm -f $MCN/cpu.conf.mcn
  return $rc
}

# Replace file $1 by the output of the command that follows. The new content is written in
# full next to it, with the old file's mode, and only then renamed over it: a power cut leaves
# either the old or the new file, never a half-written one.
mcn_rewrite() {
  f=$1
  shift
  cp -p "$f" "$f.mcn" && "$@" > "$f.mcn" && [ -s "$f.mcn" ] && mv -f "$f.mcn" "$f"
  rc=$?
  rm -f "$f.mcn"
  return $rc
}
