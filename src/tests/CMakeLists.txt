find_package(Qt6 COMPONENTS Test REQUIRED)

# add_qt_test(<name> <sources...>)
# Builds one Qt Test executable and registers it with CTest.
macro(add_qt_test name)
    add_executable(${name} ${ARGN})
    target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
    target_link_libraries(${name} PRIVATE Qt6::Core Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
endmacro()

add_qt_test(tst_serverFiles
    tst_serverFiles.cpp
    ../serverFiles.h
    ../appConfig.h
    ../appConfig.cpp
)

add_qt_test(tst_serverOutput
    tst_serverOutput.cpp
    ../serverOutput.h
)

add_qt_test(tst_appImageUtils
    tst_appImageUtils.cpp
    ../cli/appImageUtils.h
)

add_qt_test(tst_mapTypes
    tst_mapTypes.cpp
    ../mapTypes.h
)
