add_executable(lf-text-layout-tests
        LFTextLayoutTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../core/text/LFTextLayout.cpp
)

set_target_properties(lf-text-layout-tests PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

target_include_directories(lf-text-layout-tests
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../core
)

add_test(
        NAME leaf-text-layout
        COMMAND lf-text-layout-tests
)
