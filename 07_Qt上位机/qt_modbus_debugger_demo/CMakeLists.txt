cmake_minimum_required(VERSION 3.16)

project(qt_modbus_debugger_demo VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 QUIET COMPONENTS Widgets SerialPort)

if(Qt6_FOUND)
    set(QT_LIBS Qt6::Widgets Qt6::SerialPort)
else()
    find_package(Qt5 REQUIRED COMPONENTS Widgets SerialPort)
    set(QT_LIBS Qt5::Widgets Qt5::SerialPort)
endif()

add_executable(qt_modbus_debugger_demo
    src/main.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/modbus_crc.h
    src/modbus_crc.cpp
    src/modbus_frame.h
    src/modbus_frame.cpp
)

target_link_libraries(qt_modbus_debugger_demo PRIVATE ${QT_LIBS})
