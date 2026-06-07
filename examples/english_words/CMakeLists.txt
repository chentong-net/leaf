if (NOT TARGET leaf-core)
    message(FATAL_ERROR "english-words requires leaf-core target")
endif()

add_library(english-words STATIC
        EnglishWordsApp.cpp
        EnglishWordsDataManager.cpp
        ExamPage.cpp
        MainPage.cpp
        PointPage.cpp
        ResultPage.cpp
        StudyPage.cpp
        TestPage.cpp
        TopicPage.cpp
)

set_target_properties(english-words PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(english-words
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(english-words
        PUBLIC
        leaf-core
        audio-player
        i18n
        local-time
        path-provider
)
