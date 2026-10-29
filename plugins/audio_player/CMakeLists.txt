if (NOT TARGET leaf-core)
    message(FATAL_ERROR "audio-player requires leaf-core target")
endif()

if (LEAF_PLATFORM_DEF STREQUAL "WINDOWS")
    add_library(audio-player STATIC
            desktop/LFAudioPlayerDesktop.cpp
    )
elseif (LEAF_PLATFORM_DEF STREQUAL "MACOS")
    add_library(audio-player STATIC
            desktop/LFAudioPlayerDesktopMac.mm
    )
elseif (LEAF_PLATFORM_DEF STREQUAL "ANDROID")
    add_library(audio-player STATIC
            LFAudioPlayer.cpp
    )
else()
    add_library(audio-player STATIC
            LFAudioPlayer.cpp
    )
endif()

set_target_properties(audio-player PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(audio-player
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(audio-player
        PRIVATE
        leaf-core
)

if (LEAF_PLATFORM_DEF STREQUAL "WINDOWS")
    target_link_libraries(audio-player
            PRIVATE
            winmm
            ole32
            uuid
    )
endif()

if (LEAF_PLATFORM_DEF STREQUAL "MACOS")
    set_target_properties(audio-player PROPERTIES
            COMPILE_FLAGS "-fobjc-arc"
    )
    target_link_libraries(audio-player
            PRIVATE
            "-framework AVFoundation"
            "-framework Foundation"
    )
endif()
