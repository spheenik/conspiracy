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
        linux_audio_stub.cpp
        ../port/windows.c
        )

add_executable(chaos-theory ${SOURCE_FILES})
target_include_directories(chaos-theory PUBLIC ../port/include MinimalEngine ChaosTheory /usr/include/cairo)
target_compile_definitions(chaos-theory PUBLIC CONSPIRACY_LINUX)
set_target_properties(chaos-theory PROPERTIES
        COMPILE_FLAGS "-m32 -fpermissive -fno-operator-names -w"
        LINK_FLAGS "-m32 -no-pie")
target_link_libraries(chaos-theory X11 GL GLU pulse pulse-simple pthread cairo jpeg)
