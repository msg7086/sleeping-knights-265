#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

BUILD_DIR="$PROJECT_ROOT/build/ffmpeg"
PREFIX_DIR="$PROJECT_ROOT/.deps/ffmpeg"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

"$PROJECT_ROOT/third_party/ffmpeg/configure" \
    --prefix="$PREFIX_DIR" \
    --enable-static \
    --disable-shared \
    --enable-pic \
    --disable-programs \
    --disable-doc \
    --disable-debug \
    --disable-autodetect \
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
    --cc=gcc \
    --cxx=g++

make -j$(nproc)
make install
echo "Linux FFmpeg static build complete!"
