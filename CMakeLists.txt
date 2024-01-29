cmake_minimum_required(VERSION 3.14)

project(TraidingCatBot LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network)

add_executable(TraidingCatBot
  main.cpp
  httpsslquery.cpp
  httpsslquery.h
  core.cpp
  core.h
  mexc.h
  mexc.cpp
)
target_link_libraries(TraidingCatBot Qt${QT_VERSION_MAJOR}::Core Qt6::Network)

include(GNUInstallDirs)
install(TARGETS TraidingCatBot
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)