cmake_minimum_required(VERSION 3.16)

project(v4l2-test-gui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON) 

find_package(Qt6 REQUIRED COMPONENTS 
    Widgets 
    Network
)
find_package(OpenCV REQUIRED)

set(UI
    mainwindow.ui
)

set(SOURCES
    main.cpp
    window.cpp
    mainwindow.cpp
    convert.cpp
    socketserver.cpp
)

set(HEADERS
    window.hpp
    mainwindow.hpp
    convert.hpp
    socketserver.hpp
)

add_executable(${PROJECT_NAME} ${UI} ${SOURCES} ${HEADERS})
message(${PROJECT_BINARY_DIR}/src)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_BINARY_DIR}/..)

target_link_libraries(${PROJECT_NAME} 
    common
    Qt6::Widgets 
    Qt6::Network
    ${OpenCV_LIBS})