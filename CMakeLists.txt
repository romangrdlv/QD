set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

set(PROJECT_SOURCES
        source/main.cpp
        source/decoder.cpp
        source/decoder.h
        source/decoderpopup.cpp
        source/decoderpopup.h
        source/decoderpopup.ui
        source/mainwindow.cpp
        source/mainwindow.h
        source/mainwindow.ui
        source/ico.qrc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(qdd
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(qdd SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(qdd
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(qdd PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(qdd PROPERTIES
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(qdd)
endif()
