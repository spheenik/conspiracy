enable_language(ASM_NASM)

set(CMAKE_ASM_NASM_OBJECT_FORMAT elf)
set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> <INCLUDES> -f ${CMAKE_ASM_NASM_OBJECT_FORMAT} -o <OBJECT> <SOURCE>")

set(SOURCE_FILES
        cloth.cpp
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
add_executable(project-genesis ${SOURCE_FILES})
target_include_directories(project-genesis PUBLIC ../port/include)
target_compile_definitions(project-genesis PUBLIC CONSPIRACY_LINUX)
set_target_properties(project-genesis PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32 -no-pie -z muldefs")
target_link_libraries(project-genesis X11 GL GLU cairo)