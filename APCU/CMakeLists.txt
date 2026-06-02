enable_language(ASM_NASM)

set(CMAKE_ASM_NASM_OBJECT_FORMAT elf)
set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> <INCLUDES> -f ${CMAKE_ASM_NASM_OBJECT_FORMAT} -o <OBJECT> <SOURCE>")

set(SOURCE_FILES
        inroengine.cpp
        intro.cpp
        introgen.cpp
        introinput.cpp
        introplayer.cpp
        SetupDialog.cpp
        IntroWindow.cpp
        TexGen.cpp
        Track.cpp
        vectors.cpp
        fmod/Fmusic.c
        fmod/Fsound.c
        fmod/mixer_clipcopy.c
        #fmod/mixer_fpu_ramp.c
        fmod/mixer_fpu_ramp.asm
        fmod/music_formatxm.c
        player.cpp
        synth.cpp
        musicLinux.o
        ../port/windows.c
        )

set_source_files_properties(fmod/mixer_fpu_ramp.asm PROPERTIES LANGUAGE ASM_NASM)
add_executable(apcu ${SOURCE_FILES})
target_include_directories(apcu PUBLIC ../port/include /usr/include/cairo /usr/include/pango-1.0 /usr/include/glib-2.0 /usr/lib32/glib-2.0/include /usr/include/harfbuzz)
target_compile_definitions(apcu PUBLIC CONSPIRACY_LINUX)
set_target_properties(apcu PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32 -no-pie")
target_link_libraries(apcu X11 GL GLU cairo pulse pthread pangocairo-1.0 pango-1.0 gobject-2.0 glib-2.0)