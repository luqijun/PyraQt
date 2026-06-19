find_package(${PYRAQT_QT_PACKAGE} REQUIRED COMPONENTS Test)

add_executable(pyraqt_core_tests
    unit/test_main.cpp
    unit/test_core_services.cpp
    unit/test_plugin_services.cpp
    unit/test_runtime_services.cpp
    unit/test_scripting_services.cpp
    unit/test_workspace_services.cpp
    ../resources/pyraqt_resources.qrc
)

target_link_libraries(pyraqt_core_tests
    PRIVATE
        pyraqt_core
        PyraQt::QtCore
        PyraQt::QtGui
        PyraQt::QtWidgets
        PyraQt::QtTest
)

pyraqt_set_project_warnings(pyraqt_core_tests)

add_test(NAME pyraqt_core_tests COMMAND pyraqt_core_tests)
set_tests_properties(pyraqt_core_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
