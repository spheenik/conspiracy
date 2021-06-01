add_executable(project-genesis
        cloth.cpp
        inroengine.cpp
        intro.cpp
        introgen.cpp
        introinput.cpp
        introplayer.cpp
        #IntroWindow.cpp
        #SetupDialog.cpp
        TexGen.cpp
        Track.cpp
        vectors.cpp
        fmod/Fmusic.c
        fmod/Fsound.c
        #fmod/mixer_clipcopy.c
        #fmod/mixer_fpu_ramp.c
        fmod/music_formatxm.c
        player.cpp
        synth.cpp
        )

target_include_directories(project-genesis PUBLIC ../port/include)
set_target_properties(project-genesis PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
