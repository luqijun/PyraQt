find_package(${PYRAQT_QT_PACKAGE} REQUIRED COMPONENTS Test)

add_executable(pyraqt_core_tests
    unit/test_main.cpp
    unit/test_core_services.cpp
    unit/test_plugin_services.cpp
    unit/test_runtime_services.cpp
    unit/test_scripting_services.cpp
    unit/test_ui_theme.cpp
    unit/test_cad_services.cpp
    unit/test_workspace_services.cpp
    ../src/ui/common/file_dialog_utils.cpp
    ../src/ui/common/file_dialog_utils.h
    ../src/ui/common/icon_utils.cpp
    ../src/ui/common/icon_utils.h
    ../src/ui/common/python_completion_line_edit.cpp
    ../src/ui/common/python_completion_line_edit.h
    ../src/ui/common/python_completion_text_edit.cpp
    ../src/ui/common/python_completion_text_edit.h
    ../src/ui/dialogs/command_palette_dialog.cpp
    ../src/ui/dialogs/command_palette_dialog.h
    ../src/ui/dialogs/settings_dialog.cpp
    ../src/ui/dialogs/settings_dialog.h
    ../src/ui/dialogs/python_tools_dialog.cpp
    ../src/ui/dialogs/python_tools_dialog.h
    ../src/ui/editor/editor_workspace_widget.cpp
    ../src/ui/editor/editor_workspace_widget.h
    ../src/ui/editor/editor_placeholder_widget.cpp
    ../src/ui/editor/editor_placeholder_widget.h
    ../src/ui/editor/cad_document_widget.cpp
    ../src/ui/editor/cad_document_widget.h
    ../src/ui/editor/cad_summary_widget.cpp
    ../src/ui/editor/cad_summary_widget.h
    ../src/ui/editor/occt_cad_view_widget.cpp
    ../src/ui/editor/occt_cad_view_widget.h
    ../src/ui/editor/script_editor_widget.cpp
    ../src/ui/editor/script_editor_widget.h
    ../src/ui/panels/properties/cad_properties_panel.cpp
    ../src/ui/panels/properties/cad_properties_panel.h
    ../src/ui/panels/python/python_console_widget.cpp
    ../src/ui/panels/python/python_console_widget.h
    ../src/ui/panels/files/file_browser_panel.cpp
    ../src/ui/panels/files/file_browser_panel.h
    ../src/ui/panels/plugins/plugin_manager_panel.cpp
    ../src/ui/panels/plugins/plugin_manager_panel.h
    ../src/ui/mainwindow/main_window.cpp
    ../src/ui/mainwindow/main_window.h
    ../resources/pyraqt_resources.qrc
)

target_link_libraries(pyraqt_core_tests
    PRIVATE
        pyraqt_core
        PyraQt::QtCore
        PyraQt::QtGui
        PyraQt::QtWidgets
        PyraQt::QtSvg
        PyraQt::QtTest
)

if(PYRAQT_HAS_QSCINTILLA)
    target_link_libraries(pyraqt_core_tests PRIVATE QScintilla::QScintilla)
endif()

pyraqt_set_project_warnings(pyraqt_core_tests)

add_test(NAME pyraqt_core_tests COMMAND pyraqt_core_tests)
set_tests_properties(pyraqt_core_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
