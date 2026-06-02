enable_language(ASM)

# --- mvx softsynth (Phase 2 audio) -----------------------------------------------------
# Convert the Windows COFF lib mvxPlayerLib.lib -> ELF32 with clean C symbol names, at
# build time, via objconv (override with -DOBJCONV=/path or env OBJCONV).
set(MVX_ELF ${CMAKE_CURRENT_BINARY_DIR}/mvx_elf.a)
add_custom_command(
        OUTPUT ${MVX_ELF}
        COMMAND ${CMAKE_COMMAND} -E env OBJCONV=$ENV{OBJCONV}
                bash ${CMAKE_CURRENT_SOURCE_DIR}/convert_mvx.sh ${MVX_ELF}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/convert_mvx.sh
                ${CMAKE_CURRENT_SOURCE_DIR}/ChaosTheory/mvxPlayerLib.lib
        COMMENT "Converting mvxPlayerLib.lib (COFF) -> ELF32")
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
target_include_directories(chaos-theory PUBLIC ../port/include MinimalEngine ChaosTheory /usr/include/cairo
        /usr/include/gdk-pixbuf-2.0 /usr/include/glib-2.0 /usr/lib32/glib-2.0/include /usr/include/pango-1.0 /usr/include/harfbuzz)
target_compile_definitions(chaos-theory PUBLIC CONSPIRACY_LINUX)
set_target_properties(chaos-theory PROPERTIES
        COMPILE_FLAGS "-m32 -fpermissive -fno-operator-names -w"
        LINK_FLAGS "-m32 -no-pie -Wl,--allow-multiple-definition")
target_link_libraries(chaos-theory ${MVX_ELF} X11 GL GLU pulse pulse-simple pthread cairo gdk_pixbuf-2.0 gobject-2.0 glib-2.0 pangocairo-1.0 pango-1.0 gsm)
