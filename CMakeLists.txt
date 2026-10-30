cmake_minimum_required(VERSION 3.16)
project(Qute VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets PrintSupport Network)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# --- 1. Extract Git Info and Version ---
set(APP_VERSION "0")

find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_COUNT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else()
    set(GIT_HASH "unknown")
    set(GIT_COMMIT_COUNT "0")
endif()

# --- 2. Define Executable ---
add_executable(Qute
    main.cpp
    MainWindow.h
    MainWindow.cpp
    SyntaxHighlighter.h
    SyntaxHighlighter.cpp
    CodeEditor.h
    CodeEditor.cpp
    FindReplaceDialog.h
    FindReplaceDialog.cpp
    constants.h constants.cpp
    Qute.qrc
    SettingsDialog.h
    SettingsDialog.cpp
    SettingsDialog.h SettingsDialog.cpp
    commit_message.txt
    TODO
)

target_link_libraries(Qute PRIVATE Qt6::Widgets Qt6::PrintSupport Qt6::Network)

# --- 3. Inject Preprocessor Macros with String Quoting ---
target_compile_definitions(Qute PRIVATE
    APP_VERSION=\"${APP_VERSION}\"
    GIT_HASH=\"${GIT_HASH}\"
    GIT_COMMIT_COUNT=\"${GIT_COMMIT_COUNT}\"
)