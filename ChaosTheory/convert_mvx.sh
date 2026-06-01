#!/bin/bash
# Convert ChaosTheory's mvxPlayerLib.lib (COFF) -> ELF32, renaming the MSVC-mangled entry
# points / data globals / @/?-decorated externals to clean C names so the Linux audio
# driver (linux_mvx_audio.cpp) can reference/define them (GAS can't handle ? or @ in
# symbols). Invoked by CMake; OUT is $1. Override the objconv binary via $OBJCONV.
set -e
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OBJC="${OBJCONV:-$HOME/.cache/yay/objconv/src/build/objconv}"
LIB="${MVX_LIB_IN:-$HERE/ChaosTheory/mvxPlayerLib.lib}"
OUT="${1:-mvx_elf.a}"

if [ ! -x "$OBJC" ]; then echo "objconv not found at '$OBJC' (set \$OBJCONV)"; exit 1; fi

R=()
# --- entry points / API (called by the engine via mvx_lite.h) ---
R+=("-nr:?mvxSystem_Init@@YAHPAUHWND__@@PADH@Z:mvxSystem_Init")
R+=("-nr:?mvxSystem_Play@@YAXXZ:mvxSystem_Play")
R+=("-nr:?mvxSystem_Stop@@YAXXZ:mvxSystem_Stop")
R+=("-nr:?mvxSystem_DeInit@@YAXXZ:mvxSystem_DeInit")
R+=("-nr:?mvxSystem_GetSync@@YAHXZ:mvxSystem_GetSync")
R+=("-nr:?mvxSystem_ClipBuffer@@YAXPAMPAFH@Z:mvxSystem_ClipBuffer")
R+=("-nr:?mvxMixer_Render@@YAXPAMH@Z:mvxMixer_Render")
R+=("-nr:?mvxMixer_Init@@YAXXZ:mvxMixer_Init")
R+=("-nr:?mvxMixer_DeInit@@YAXXZ:mvxMixer_DeInit")
# --- data globals ---
R+=("-nr:?mvxSystem_SongLength@@3HA:mvxSystem_SongLength")
R+=("-nr:?mvxSystem_SamplesPerTick@@3HA:mvxSystem_SamplesPerTick")
R+=("-nr:?mvxSystem_ExitThread@@3HA:mvxSystem_ExitThread")
# --- @/?-decorated externals the driver DEFINES (clean names) ---
R+=("-nr:??2@YAPAXI@Z:mvx_op_new")
R+=("-nr:??3@YAXPAX@Z:mvx_op_delete")
R+=("-nr:_DirectSoundCreate8@12:mvx_DirectSoundCreate8")
for f in acmDriverClose@8 acmDriverEnum@12 acmDriverOpen@12 acmFormatEnumA@20 acmMetrics@12 acmStreamClose@8 acmStreamConvert@12 acmStreamOpen@32 acmStreamPrepareHeader@12; do
  R+=("-nr:_${f}:mvx_${f%@*}")
done
for f in CreateThread@24 QueryPerformanceCounter@4 QueryPerformanceFrequency@4 SetThreadPriority@8 Sleep@4; do
  R+=("-nr:__imp__${f}:mvx_imp_${f%@*}")
done

rm -f "$OUT"
"$OBJC" -felf32 "${R[@]}" "$LIB" "$OUT" 2>&1 | grep -iE "error|not found" | head || true
echo "wrote $OUT"
