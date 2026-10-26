cmake_minimum_required(VERSION 3.16)
project(QtTextEditor VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(QtTextEditor
    main.cpp
    MainWindow.h
    MainWindow.cpp
    SyntaxHighlighter.h
    SyntaxHighlighter.cpp
    CodeEditor.h
    CodeEditor.cpp
    FindReplaceDialog.h
    FindReplaceDialog.cpp
)

target_link_libraries(QtTextEditor PRIVATE Qt6::Widgets)