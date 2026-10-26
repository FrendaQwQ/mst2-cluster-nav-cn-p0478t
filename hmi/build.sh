#!/bin/sh
# Build the HMI bundle jar.
#   MIBHMI=<MIBHMI.jar converted from your unit's MIBHMI.jxe> JAVAC=<JDK 8 javac> ./build.sh
# The HMI runs on J9 (jclFoundation11) with class files of version 46-49, so the classes
# are compiled for Java 1.3; JDK 8 is the newest javac that still accepts that.
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
: "${MIBHMI:?set MIBHMI to the converted MIBHMI.jar}"
JAVAC="${JAVAC:-javac}"
JAR="${JAR:-$(dirname "$JAVAC")/jar}"
OUT="$HERE/build"
rm -rf "$OUT"
mkdir -p "$OUT/classes"
"$JAVAC" -source 1.3 -target 1.3 -nowarn -encoding UTF-8 -cp "$MIBHMI" \
    -d "$OUT/classes" "$HERE"/src/mst2/clusternav/*.java
"$JAR" cf "$OUT/mst2-cluster-nav.jar" -C "$OUT/classes" .
echo "$OUT/mst2-cluster-nav.jar"
