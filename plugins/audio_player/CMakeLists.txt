if (NOT TARGET leaf-core)
    message(FATAL_ERROR "audio-player requires leaf-core target")
endif()

if (LEAF_PLATFORM_DEF STREQUAL "DESKTOP")
    add_library(audio-player STATIC
            desktop/LFAudioPlayerDesktop.cpp
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

if (LEAF_PLATFORM_DEF STREQUAL "DESKTOP" AND WIN32)
    target_link_libraries(audio-player
            PRIVATE
            winmm
            ole32
            uuid
    )
endif()
