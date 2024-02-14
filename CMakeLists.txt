cmake_minimum_required(VERSION 3.14)

project(TraidingCatBot LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network Sql)

set(SOURCE_FILES
    main.cpp
    core.cpp
    core.h
    mexckline.h
    mexckline.cpp
    config.cpp
    config.h
    types.h
    types.cpp
    money.cpp
    money.h
    stockexchange.cpp
    stockexchange.h
    mexcstockexchange.h
    mexcstockexchange.cpp
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Sql)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/../../Common/Headers)
target_link_libraries(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/../../Common/Lib/libCommon.a)

include(GNUInstallDirs)
install(TARGETS TraidingCatBot
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
