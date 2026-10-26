#!/bin/sh
# Build the mod and lay out the toolbox SD card: build/sd/custom/ goes to the root of the card
# next to the MIB STD2 toolbox's own custom/ directory, then GEM -> Customization -> Cluster
# navigation -> Install.
#   MIBHMI=<MIBHMI.jar converted from your unit's MIBHMI.jxe> JAVAC=<JDK 8 javac> ./package.sh
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
"$HERE/hmi/build.sh"
"$HERE/native/build.sh"
SD="$HERE/build/sd"
rm -rf "$SD"
mkdir -p "$SD"
cp -R "$HERE/toolbox/custom" "$SD/custom"
cp "$HERE/hmi/build/mst2-cluster-nav.jar" "$SD/custom/mst2-cluster-nav/"
for lib in $(awk '!/^#/ && NF == 2 { print $2 }' "$HERE/toolbox/custom/mst2-cluster-nav/preload.list"); do
    cp "$HERE/native/build/$lib" "$SD/custom/mst2-cluster-nav/"
done
(cd "$HERE/build" && rm -f mst2-cluster-nav-sd.zip && zip -qr mst2-cluster-nav-sd.zip sd)
echo "$HERE/build/mst2-cluster-nav-sd.zip"
