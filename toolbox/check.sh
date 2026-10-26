#!/bin/sh
# Host check of the runHMI.sh guard and the installer's conflict check, run from the guard as it
# sits inside runHMI.sh (the unit's paths mapped into a temporary directory). A synthetic
# runHMI.sh stands in for the stock one.
set -eu
HERE="$(cd "$(dirname "$0")" && pwd)"
T="$(mktemp -d)"
trap 'rm -rf "$T"' EXIT
BLOCK="$HERE/custom/mst2-cluster-nav/runHMI.block"
STRIP="$(sed -n "s/^  sed '\(.*\)' \"\$1\"$/\1/p" "$HERE/custom/greenmenu/scripts/mcn_common.sh")" # mcn_strip_block's
printf '%s\n' '#!/bin/ksh' 'BOOTCLASSPATH="$IFSDIR/tsd.mibstd2.hmi.v2.jxe:$J9LIB/x.zip"' \
    'echo run' > "$T/stock.sh"
sed '/^BOOTCLASSPATH="\$IFSDIR\/tsd.mibstd2.hmi.v2.jxe:/r '"$BLOCK" "$T/stock.sh" > "$T/mod.sh"
sed 's#^\(BOOTCLASSPATH=.*\)$#\1\
BOOTCLASSPATH="$BOOTCLASSPATH -Xbootclasspath/p:$MIBJAR/NavActiveIgnore.jar"#' "$T/mod.sh" \
    > "$T/navignore.sh"

fail=0
# guard <runHMI.sh> <setup> <expect loaded y|n> <expect reason> <name>
guard() {
    W="$T/w"
    rm -rf "$W"
    mkdir -p "$W/var" "$W/shm" "$W/media/mp000" "$W/lib"
    echo x > "$W/var/mst2-cluster-nav.jar"
    echo x > "$W/lib/libmcn_sal.so"
    echo "   envar LD_PRELOAD $W/lib/libmcn_sal.so" > "$W/cpu.conf"
    cp "$1" "$W/runHMI.sh"
    (cd "$W" && eval "$2")
    sed -n '/^# >>> mst2-cluster-nav/,/^# <<< mst2-cluster-nav/p' "$W/runHMI.sh" | sed -e "
        s#/tsd/var/mst2-cluster-nav#$W/var#g; s#/dev/shmem#$W/shm#g
        s#/media/mp00?#$W/media/mp00?#g; s#/tsd/etc/system/cpu.conf#$W/cpu.conf#
        s#/tsd/hmi/runHMI.sh#$W/runHMI.sh#; s#/tsd/\[a-z\]\*/mst2-cluster-nav#$W/lib#g
        s#/usr/bin/##g; s#/bin/rm#rm#g; s#/bin/mount#false#g" > "$W/block.sh"
    BOOTCLASSPATH=stock
    . "$W/block.sh" > /dev/null
    loaded=n
    [ "$BOOTCLASSPATH" != stock ] && loaded=y
    reason="$(cat "$W/var/off-reason" 2>/dev/null || true)"
    if [ "$loaded" != "$3" ] || [ "$reason" != "$4" ]; then
        echo "guard FAILED ($5): loaded=$loaded reason=$reason"
        fail=1
    fi
}
guard "$T/mod.sh" 'touch var/enabled' y '' 'enabled, the block inside runHMI.sh'
guard "$T/mod.sh" 'touch var/trial' y '' 'trial boot'
guard "$T/mod.sh" 'touch var/trial shm/mst2-cluster-nav-armed' n '' 'trial armed on this boot'
guard "$T/mod.sh" 'touch var/enabled media/mp000/mst2-cluster-nav-disable' n \
    "kill switch on $T/w/media/mp000" 'kill switch'
guard "$T/mod.sh" 'touch var/enabled shm/mst2-cluster-nav-on' n \
    'the HMI restarted before the mod settled' 'HMI restart before settling'
guard "$T/navignore.sh" 'touch var/enabled' n 'the toolbox dual navigation patch is active' \
    'dual navigation added later'
guard "$T/mod.sh" 'touch var/enabled; : > lib/libmcn_sal.so' n \
    "$T/w/lib/libmcn_sal.so missing: removed from cpu.conf" 'empty preload library'

# The installer's conflict check (mcn_install.sh): only lines outside our block count.
sed "$STRIP" "$T/mod.sh" | grep -q 'NavActiveIgnore.jar' && { echo "install check FAILED: own block"; fail=1; }
sed "$STRIP" "$T/navignore.sh" | grep -q 'NavActiveIgnore.jar' || { echo "install check FAILED: patch"; fail=1; }

# Uninstall (mcn_strip_block) gives the stock file back byte for byte.
sed "$STRIP" "$T/mod.sh" | cmp -s - "$T/stock.sh" || { echo "uninstall FAILED: not stock"; fail=1; }
[ -n "$STRIP" ] || { echo "check FAILED: mcn_strip_block not found"; fail=1; }

# GEM runs a script by its path alone (no arguments, as in every Toolbox screen): each menu
# entry names one packaged script, without spaces.
for path in $(sed -n 's/^ *value *sys 1 0x0100 "\(.*\)"$/\1/p' "$HERE/custom/greenmenu/"*.esd | tr ' ' '?'); do
    case "$path" in
    *'?'*) echo "esd FAILED: arguments in $path"; fail=1 ;;
    /tsd/etc/persistence/esd/scripts/*)
        [ -f "$HERE/custom/greenmenu/scripts/${path##*/}" ] || { echo "esd FAILED: $path missing"; fail=1; } ;;
    esac
done

[ "$fail" = 0 ] && echo "toolbox: runHMI.sh guard, install conflict check and menu scripts: ok"
exit "$fail"
