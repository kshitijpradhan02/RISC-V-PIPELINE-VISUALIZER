cmake_minimum_required(VERSION 3.16)

project(RISCV_Pipeline VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 COMPONENTS
    Core
    Gui
    Widgets
    REQUIRED
)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.h
    mainwindow.cpp
    pipeline.h
    pipeline.cpp
)

add_executable(RISCV_Pipeline
    ${PROJECT_SOURCES}
)

target_link_libraries(RISCV_Pipeline
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)
