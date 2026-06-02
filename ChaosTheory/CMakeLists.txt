enable_language(ASM)

# --- vendored libgsm (GSM 6.10 decode), built isolated with clean C flags ------------
file(GLOB GSM_SOURCES ${CMAKE_SOURCE_DIR}/third_party/gsm/*.c)
add_library(gsm_vendor STATIC ${GSM_SOURCES})
target_include_directories(gsm_vendor PRIVATE ${CMAKE_SOURCE_DIR}/third_party/gsm)
# SASR (arithmetic >>) and WAV49 (MS-GSM 65-byte blocks) on the command line, matching the
# upstream Makefile. These MUST be compile flags, not only config.h #defines: CMake doesn't
# track config.h (included via private.h) as a dependency, so a config.h-only define can go
# stale and silently disable WAV49 -> the drum/bass GSM samples decode to garbage. -O2 matches
# the system libgsm build.
set_target_properties(gsm_vendor PROPERTIES COMPILE_FLAGS "-m32 -O2 -DSASR -DWAV49" LINK_FLAGS "-m32")

# --- mvx softsynth (Phase 2 audio) -----------------------------------------------------
# 1) Convert the Windows COFF lib mvxPlayerLib.lib -> ELF32 with clean C symbol names, at
#    build time, via objconv (override with -DOBJCONV=/path or env OBJCONV).
set(MVX_ELF_RAW ${CMAKE_CURRENT_BINARY_DIR}/mvx_elf.a)
add_custom_command(
        OUTPUT ${MVX_ELF_RAW}
        COMMAND ${CMAKE_COMMAND} -E env OBJCONV=$ENV{OBJCONV}
                bash ${CMAKE_CURRENT_SOURCE_DIR}/convert_mvx.sh ${MVX_ELF_RAW}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/convert_mvx.sh
                ${CMAKE_CURRENT_SOURCE_DIR}/ChaosTheory/mvxPlayerLib.lib
        COMMENT "Converting mvxPlayerLib.lib (COFF) -> ELF32")

# 2) Resolve duplicate ELF symbols from the COFF->ELF conversion so we can link WITHOUT
#    -Wl,--allow-multiple-definition (whose order-dependent "first wins" silently mis-wired
#    per-plugin methods -> e.g. no percussion). fix_mvx.sh de-versions the MSVC '@@'-mangled
#    names (ld reads '@@' as symbol versioning) and weakens the identical COMDAT data so the
#    link is deterministic and order-independent. See fix_mvx.sh for the full rationale.
set(MVX_ELF ${CMAKE_CURRENT_BINARY_DIR}/mvx_elf_fixed.a)
add_custom_command(
        OUTPUT ${MVX_ELF}
        COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/fix_mvx.sh ${MVX_ELF_RAW} ${MVX_ELF}
        DEPENDS ${MVX_ELF_RAW} ${CMAKE_CURRENT_SOURCE_DIR}/fix_mvx.sh
        COMMENT "Resolving mvx COMDAT/@-versioning duplicates (fix_mvx.sh)")
add_custom_target(mvx_convert DEPENDS ${MVX_ELF})

# Embed music.mvm as `music` / `music_size` (fill the .incbin path for this checkout).
set(CT_MUSIC_MVM ${CMAKE_CURRENT_SOURCE_DIR}/ChaosTheory/Binary/music.mvm)
configure_file(linux_mvx_blob.s.in ${CMAKE_CURRENT_BINARY_DIR}/linux_mvx_blob.s @ONLY)

set(SOURCE_FILES
        MinimalEngine/3dEngine.cpp
        MinimalEngine/AdvancedMesh.cpp
        MinimalEngine/cubes.cpp
        MinimalEngine/EventHandler.cpp
        MinimalEngine/IntroEditorEffects.cpp
        MinimalEngine/ObjectGenerators.cpp
        MinimalEngine/Parser.cpp
        MinimalEngine/ParticleEngine.cpp
        MinimalEngine/Splines.cpp
        MinimalEngine/TexGen.cpp
        MinimalEngine/Vectors.cpp
        MinimalEngine/WindowHandler.cpp
        MinimalEngine/writer.cpp
        ChaosTheory/aDDict2Engine.cpp
        ChaosTheory/SetupDialog.cpp
        linux_mvx_audio.cpp
        linux_mvx_fpu.s
        ${CMAKE_CURRENT_BINARY_DIR}/linux_mvx_blob.s
        ../port/windows.c
        )

add_executable(chaos-theory ${SOURCE_FILES})
add_dependencies(chaos-theory mvx_convert)
# the assembler files are 32-bit AT&T GAS
set_source_files_properties(linux_mvx_fpu.s ${CMAKE_CURRENT_BINARY_DIR}/linux_mvx_blob.s
        PROPERTIES COMPILE_FLAGS "-m32")
target_include_directories(chaos-theory PUBLIC ../port/include MinimalEngine ChaosTheory ${CMAKE_SOURCE_DIR}/third_party)
target_compile_definitions(chaos-theory PUBLIC CONSPIRACY_LINUX)
set_target_properties(chaos-theory PROPERTIES
        COMPILE_FLAGS "-m32 -fpermissive -fno-operator-names -w"
        LINK_FLAGS "-m32 -no-pie")
target_link_libraries(chaos-theory ${MVX_ELF} X11 GL GLU pulse pulse-simple pthread gsm_vendor)
