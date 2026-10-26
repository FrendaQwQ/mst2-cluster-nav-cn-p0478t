#!/bin/sh
# Build the native hook libraries for QNX 6.5 armle-v7 on any host with clang and ld.lld
# (macOS: brew install llvm lld). No QNX SDK and no files from the unit are needed: the
# unit's libc.so.3 and libsocket.so.3 are replaced at link time by stubs that only carry the
# symbol names. Each directory src/<name>/ becomes libmcn_<name>.so, with src/common/ linked into
# every one; its entry point is
# mcn_init, called through DT_INIT: the QNX 6.5 run-time linker predates .init_array, so no
# constructors are used.
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
LLVM="${LLVM:-$(brew --prefix llvm 2>/dev/null)/bin}"
CLANG="${CLANG:-$LLVM/clang}"
LLD="${LLD:-$(brew --prefix lld 2>/dev/null)/bin/ld.lld}"
OUT="$HERE/build"
CFLAGS="--target=armv7a-none-eabi -marm -mfloat-abi=soft -fPIC -Os -ffreestanding -nostdlib
 -fno-stack-protector -fno-builtin -fvisibility=hidden -std=c99 -Wall -Wextra -Werror
 -I$HERE/include"
LDFLAGS="-shared -z defs -z max-page-size=0x1000 --hash-style=sysv --gc-sections"

rm -rf "$OUT"
mkdir -p "$OUT/stubs"
for stub in "$HERE"/stubs/*_stub.c; do
    lib=$(basename "$stub" _stub.c)
    "$CLANG" $CFLAGS -fvisibility=default -c "$stub" -o "$OUT/stubs/$lib.o"
    "$LLD" -shared --hash-style=sysv -soname "$lib.so.3" -o "$OUT/stubs/$lib.so.3" "$OUT/stubs/$lib.o"
done

for dir in "$HERE"/src/*/; do
    name=$(basename "$dir")
    [ "$name" = common ] && continue
    objs=""
    for src in "$dir"*.c "$HERE"/src/common/*.c; do
        obj="$OUT/$name-$(basename "$(dirname "$src")")-$(basename "$src" .c).o"
        "$CLANG" $CFLAGS -c "$src" -o "$obj"
        objs="$objs $obj"
    done
    "$LLD" $LDFLAGS -soname "libmcn_$name.so" -init mcn_init -o "$OUT/libmcn_$name.so" $objs \
        "$OUT"/stubs/*.so.3
    echo "$OUT/libmcn_$name.so"
done
