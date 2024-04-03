cmake_minimum_required(VERSION 3.14)

project(TradingCat LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network Sql HttpServer)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network Sql HttpServer)

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
    mexcstockexchange.h
    mexcstockexchange.cpp
    dbsaver.h
    dbsaver.cpp
)

SET(COMMON_FILES
    ${CMAKE_SOURCE_DIR}/../../Common/Common/common.h
    ${CMAKE_SOURCE_DIR}/../../Common/Common/common.cpp
    ${CMAKE_SOURCE_DIR}/../../Common/Common/httpsslquery.h
    ${CMAKE_SOURCE_DIR}/../../Common/Common/httpsslquery.cpp
    ${CMAKE_SOURCE_DIR}/../../Common/Common/tdbloger.h
    ${CMAKE_SOURCE_DIR}/../../Common/Common/tdbloger.cpp
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${COMMON_FILES}
    appserver.h appserver.cpp
    history.h history.cpp
    filter.h filter.cpp
    log.h log.cpp
    deamon.h deamon.cpp
    usersonline.h usersonline.cpp
    kucoinstockexchange.h kucoinstockexchange.cpp
    kucoinkline.h kucoinkline.cpp
    gatekline.h gatekline.cpp
    gatestockexchange.h gatestockechan.cpp)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Network
        Qt${QT_VERSION_MAJOR}::Sql
        Qt${QT_VERSION_MAJOR}::HttpServer
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/../../Common)

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install the config file
#install(FILES resource/config/daemon-template.conf DESTINATION /etc/daemon-template)

# Intall the systemd file
#install(FILES resource/systemd/daemon-template.service DESTINATION /etc/systemd/system)

# Install the binary program
#install(TARGETS ${THIS} DESTINATION /usr/bin)
