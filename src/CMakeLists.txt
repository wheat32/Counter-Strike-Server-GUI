cmake_minimum_required(VERSION 3.16)
project(CSServerManager)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        Svg
        SvgWidgets
        REQUIRED)

set(SOURCES
        main.cpp
        appConfig.h
        appConfig.cpp
        themeManager.h
        themeManager.cpp
        debug.h
        uiHelpers.h
        cli/appImageUtils.h
        cli/flatpakUtils.h
        cli/platformUtils.h
        widgets/elidaLabel.h
        widgets/infoBanner.h
        widgets/numberSpinner.h
        widgets/svgBanner.h
        widgets/toastNotification.h
        widgets/toggleSwitch.h
        widgets/toggleSwitch.cpp
        widgets/toggleWithStatus.h
        resources.qrc
)

add_executable(cs_server_manager ${SOURCES})

target_link_libraries(cs_server_manager
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Svg
        Qt6::SvgWidgets
)

install(TARGETS cs_server_manager
        RUNTIME DESTINATION bin
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../cs-server-manager.desktop
        DESTINATION share/applications
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../cs-server-manager.svg
        DESTINATION share/icons/hicolor/scalable/apps
)
