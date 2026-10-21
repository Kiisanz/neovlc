
cmake_minimum_required(VERSION 3.16)

project(neovlc VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Quick QuickControls2)

set(CMAKE_AUTOMOC ON)

qt_add_executable(neovlc main.cpp)

qt_add_qml_module(neovlc
    URI NeoVLC
    VERSION 1.0
    RESOURCE_PREFIX /
    QML_FILES qml/Main.qml
)

target_link_libraries(neovlc
    PRIVATE Qt6::Quick Qt6::QuickControls2
)
