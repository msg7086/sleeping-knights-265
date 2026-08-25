#!/usr/bin/env bash
set -e

cd "$(dirname "$0")/.."

CACHE_DIR="$(pwd)/.deps/x265"
WORK_DIR="$(pwd)/build/x265"
SOURCE_DIR="$(pwd)/third_party/x265/source"

mkdir -p "$CACHE_DIR/8bit" "$CACHE_DIR/10bit" "$CACHE_DIR/12bit"
mkdir -p "$WORK_DIR/8bit_build" "$WORK_DIR/10bit_build" "$WORK_DIR/12bit_build"

echo "=== [x265 Win] Step 1: Configuring 8-bit, 10-bit, and 12-bit in parallel ==="
cmake -S "$SOURCE_DIR" -B "$WORK_DIR/8bit_build" -G Ninja \
    -DHIGH_BIT_DEPTH=OFF -DEXPORT_C_API=OFF -DENABLE_SHARED=OFF -DENABLE_CLI=OFF -DCMAKE_BUILD_TYPE=Release &
PID_CFG_8=$!

cmake -S "$SOURCE_DIR" -B "$WORK_DIR/10bit_build" -G Ninja \
    -DHIGH_BIT_DEPTH=ON -DMAIN12=OFF -DEXPORT_C_API=OFF -DENABLE_SHARED=OFF -DENABLE_CLI=OFF -DCMAKE_BUILD_TYPE=Release &
PID_CFG_10=$!

cmake -S "$SOURCE_DIR" -B "$WORK_DIR/12bit_build" -G Ninja \
    -DHIGH_BIT_DEPTH=ON -DMAIN12=ON -DEXPORT_C_API=OFF -DENABLE_SHARED=OFF -DENABLE_CLI=OFF -DCMAKE_BUILD_TYPE=Release &
PID_CFG_12=$!

wait $PID_CFG_8
wait $PID_CFG_10
wait $PID_CFG_12

echo "=== [x265 Win] Step 2: Building 8-bit, 10-bit, and 12-bit x265 in parallel ==="
cmake --build "$WORK_DIR/8bit_build" --target x265-static &
PID_BLD_8=$!

cmake --build "$WORK_DIR/10bit_build" --target x265-static &
PID_BLD_10=$!

cmake --build "$WORK_DIR/12bit_build" --target x265-static &
PID_BLD_12=$!

wait $PID_BLD_8
wait $PID_BLD_10
wait $PID_BLD_12

echo "=== [x265 Win] Step 3: Copying build artifacts to cache ==="
cp "$WORK_DIR/8bit_build/libx265.a" "$CACHE_DIR/8bit/libx265.a"
cp "$WORK_DIR/8bit_build/x265_config.h" "$CACHE_DIR/8bit/x265_config.h"

cp "$WORK_DIR/10bit_build/libx265.a" "$CACHE_DIR/10bit/libx265.a"
cp "$WORK_DIR/10bit_build/x265_config.h" "$CACHE_DIR/10bit/x265_config.h"

cp "$WORK_DIR/12bit_build/libx265.a" "$CACHE_DIR/12bit/libx265.a"
cp "$WORK_DIR/12bit_build/x265_config.h" "$CACHE_DIR/12bit/x265_config.h"

echo "=== [x265 Win] Multi-depth x265 build completed successfully! ==="
