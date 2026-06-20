find_package(${PYRAQT_QT_PACKAGE} REQUIRED COMPONENTS Test)

add_executable(pyraqt_core_tests
    unit/test_main.cpp
    unit/test_core_services.cpp
    unit/test_plugin_services.cpp
    unit/test_runtime_services.cpp
    unit/test_scripting_services.cpp
    unit/test_ui_theme.cpp
    unit/test_modeling_services.cpp
    unit/test_workspace_services.cpp
    ../src/ui/common/file_dialog_utils.cpp
    ../src/ui/common/file_dialog_utils.h
    ../src/ui/editor/editor_workspace_widget.cpp
    ../src/ui/editor/editor_workspace_widget.h
    ../src/ui/editor/model_document_widget.cpp
    ../src/ui/editor/model_document_widget.h
    ../src/ui/editor/model_summary_widget.cpp
    ../src/ui/editor/model_summary_widget.h
    ../src/ui/editor/occt_model_view_widget.cpp
    ../src/ui/editor/occt_model_view_widget.h
    ../src/ui/editor/script_editor_widget.cpp
    ../src/ui/editor/script_editor_widget.h
    ../src/ui/panels/properties/model_properties_panel.cpp
    ../src/ui/panels/properties/model_properties_panel.h
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

if(PYRAQT_HAS_QSCINTILLA)
    target_link_libraries(pyraqt_core_tests PRIVATE QScintilla::QScintilla)
endif()

pyraqt_set_project_warnings(pyraqt_core_tests)

add_test(NAME pyraqt_core_tests COMMAND pyraqt_core_tests)
set_tests_properties(pyraqt_core_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
