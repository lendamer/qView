# Compile the same application implementation into each real behavior test.
find_package(Qt6 REQUIRED COMPONENTS Test)
set(TEST_APP_SOURCES ${PROJECT_SOURCES} ${PROJECT_HEADERS})
list(REMOVE_ITEM TEST_APP_SOURCES src/main.cpp)
list(TRANSFORM TEST_APP_SOURCES PREPEND "${PROJECT_SOURCE_DIR}/")
set(CMAKE_AUTOUIC_SEARCH_PATHS "${PROJECT_SOURCE_DIR}/src")
get_target_property(APP_COMPILE_DEFINITIONS qView COMPILE_DEFINITIONS)

foreach(test_name IN ITEMS actionmanager videoplayback mediaintegration)
    if(test_name STREQUAL "actionmanager")
        set(test_source tst_actionmanagertests.cpp)
        set(test_case ActionManagerTests)
    else()
        set(test_source tst_${test_name}.cpp)
        set(test_case ${test_name})
    endif()
    add_executable(${test_name}tests ${test_source} ${TEST_APP_SOURCES}
        "${PROJECT_SOURCE_DIR}/resources/resources.qrc")
    target_include_directories(${test_name}tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
    target_compile_definitions(${test_name}tests PRIVATE ${APP_COMPILE_DEFINITIONS}
        "TEST_FIXTURE_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/fixtures\"")
    target_link_libraries(${test_name}tests PRIVATE
        Qt6::Core Qt6::Gui Qt6::Network Qt6::Widgets Qt6::Svg Qt6::Concurrent
        Qt6::Multimedia Qt6::MultimediaWidgets Qt6::Test ${PLATFORM_SPECIFIC_LIBS})
    add_test(NAME ${test_case} COMMAND ${test_name}tests)
    set_tests_properties(${test_case} PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE)
endforeach()
