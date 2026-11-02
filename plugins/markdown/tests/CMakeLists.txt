add_executable(markdown-tests
        LFMarkdownTests.cpp
)

set_target_properties(markdown-tests PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

target_link_libraries(markdown-tests
        PRIVATE
        markdown
)
