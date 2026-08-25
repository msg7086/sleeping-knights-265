#!/usr/bin/env bash
set -e

cd "$(dirname "$0")/.."

BUILD_DIR="$(pwd)/build/ffmpeg"
PREFIX_DIR="$(pwd)/.deps/ffmpeg"
SOURCE_DIR="$(pwd)/third_party/ffmpeg"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

"$SOURCE_DIR/configure" \
    --prefix="$PREFIX_DIR" \
    --enable-static \
    --disable-shared \
    --enable-pic \
    --disable-programs \
    --disable-doc \
    --disable-debug \
    --disable-autodetect \
    --disable-clock_gettime \
    --disable-nanosleep \
    --enable-w32threads \
    --enable-gpl \
    --enable-version3 \
    --enable-avcodec \
    --enable-avformat \
    --enable-avutil \
    --enable-swscale \
    --enable-swresample \
    --enable-protocol=file \
    --enable-protocol=pipe \
    --enable-demuxers \
    --enable-decoders \
    --enable-muxer=matroska \
    --enable-muxer=mp4 \
    --enable-muxer=mov \
    --enable-muxer=hevc \
    --enable-muxer=h264 \
    --enable-muxer=null \
    --enable-muxer=adts \
    --enable-muxer=latm \
    --enable-muxer=flac \
    --enable-muxer=ogg \
    --enable-muxer=opus \
    --enable-muxer=ac3 \
    --enable-muxer=eac3 \
    --enable-muxer=wav \
    --enable-muxer=segment \
    --enable-parsers \
    --enable-bsfs \
    --arch=x86_64 \
    --target-os=mingw32 \
    --cross-prefix="" \
    --cc=gcc \
    --cxx=g++

make -j$(nproc)
make install
echo "FFmpeg Windows static build complete!"
