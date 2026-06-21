cmake_minimum_required(VERSION 3.21)

project(
    PyraQt
    VERSION 0.1.0
    DESCRIPTION "A modern Qt desktop application template with a phased plugin and scripting roadmap"
    LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(CompilerWarnings)
include(Qt5Qt6Compat)

option(PYRAQT_BUILD_TESTS "Build PyraQt tests" ON)
option(PYRAQT_ENABLE_SPDLOG "Use spdlog when available" ON)
option(PYRAQT_ENABLE_SCRIPTING "Build Phase 2 scripting features" ON)
option(PYRAQT_ENABLE_PLUGINS "Build Phase 3 plugin features" ON)
option(PYRAQT_ENABLE_OCCT "Enable OCCT-based model import when OpenCASCADE is available" ON)
set(PYRAQT_QT_MAJOR_VERSION "AUTO" CACHE STRING "Qt major version to use: AUTO, 5, or 6")
set_property(CACHE PYRAQT_QT_MAJOR_VERSION PROPERTY STRINGS AUTO 5 6)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

pyraqt_find_qt()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Threads REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
if(PYRAQT_ENABLE_SPDLOG)
    find_package(spdlog QUIET)
endif()

set(PYRAQT_HAS_OCCT OFF)
if(PYRAQT_ENABLE_OCCT)
    find_package(OCCT QUIET)
    if(OCCT_FOUND AND TARGET OCCT::OCCT)
        set(PYRAQT_HAS_OCCT ON)
    else()
        message(WARNING "OCCT not found; STEP/BREP model import support will be disabled.")
    endif()
endif()

set(PYRAQT_HAS_QSCINTILLA OFF)
if(PYRAQT_ENABLE_SCRIPTING AND PYRAQT_QT_VERSION_MAJOR EQUAL 5)
    find_package(QScintilla QUIET)
    if(QScintilla_FOUND)
        set(PYRAQT_HAS_QSCINTILLA ON)
    endif()
endif()

pyraqt_add_resources(PYRAQT_RESOURCE_SOURCES resources/pyraqt_resources.qrc)
pyraqt_add_translations(PYRAQT_TRANSLATION_SOURCES
    resources/translations/pyraqt_en_US.ts
    resources/translations/pyraqt_zh_CN.ts
)

add_library(pyraqt_core STATIC
    src/core/command/command_manager.cpp
    src/core/command/command_manager.h
    src/core/command/command_descriptor.h
    src/core/config/config_manager.cpp
    src/core/config/config_manager.h
    src/core/i18n/i18n_manager.cpp
    src/core/i18n/i18n_manager.h
    src/core/logging/log_manager.cpp
    src/core/logging/log_manager.h
    src/core/modeling/model_import_manager.cpp
    src/core/modeling/model_import_manager.h
    src/core/modeling/model_property_service.cpp
    src/core/modeling/model_property_service.h
    src/core/modeling/model_types.h
    src/core/plugin/iplugin.h
    src/core/plugin/plugin_context.cpp
    src/core/plugin/plugin_context.h
    src/core/plugin/plugin_manager.cpp
    src/core/plugin/plugin_manager.h
    src/core/plugin/plugin_types.h
    src/core/runtime/crash_recovery_manager.cpp
    src/core/runtime/crash_recovery_manager.h
    src/core/scripting/pyra_api_bridge.cpp
    src/core/scripting/pyra_api_bridge.h
    src/core/scripting/python_completion_provider.cpp
    src/core/scripting/python_completion_provider.h
    src/core/scripting/python_feature_manager.cpp
    src/core/scripting/python_feature_manager.h
    src/core/scripting/python_runner.cpp
    src/core/scripting/python_runner.h
    src/core/scripting/python_runtime_manager.cpp
    src/core/scripting/python_runtime_manager.h
    src/core/scripting/script_execution_manager.cpp
    src/core/scripting/script_execution_manager.h
    src/core/scripting/script_process_runner.cpp
    src/core/scripting/script_process_runner.h
    src/core/scripting/script_result.h
    src/core/scripting/script_session.h
    src/core/theme/theme_manager.cpp
    src/core/theme/theme_manager.h
    src/core/update/update_manager.cpp
    src/core/update/update_manager.h
    src/core/update/update_types.h
    src/core/workspace/workspace_manager.cpp
    src/core/workspace/workspace_manager.h
    src/core/workspace/workspace_types.h
)

target_include_directories(pyraqt_core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(pyraqt_core
    PUBLIC
        PyraQt::QtCore
        PyraQt::QtGui
        PyraQt::QtWidgets
        PyraQt::QtSvg
        PyraQt::QtOpenGLWidgets
        Python3::Python
    PRIVATE
        Threads::Threads
)

if(spdlog_FOUND)
    target_link_libraries(pyraqt_core PRIVATE spdlog::spdlog)
    target_compile_definitions(pyraqt_core PRIVATE PYRAQT_HAS_SPDLOG=1)
endif()

if(PYRAQT_HAS_OCCT)
    target_link_libraries(pyraqt_core PUBLIC OCCT::OCCT)
endif()

pyraqt_set_project_warnings(pyraqt_core)

add_executable(pyraqt
    src/app/application.cpp
    src/app/application.h
    src/app/main.cpp
    src/ui/common/file_dialog_utils.cpp
    src/ui/common/file_dialog_utils.h
    src/ui/common/icon_utils.cpp
    src/ui/common/icon_utils.h
    src/ui/common/python_completion_line_edit.cpp
    src/ui/common/python_completion_line_edit.h
    src/ui/common/python_completion_text_edit.cpp
    src/ui/common/python_completion_text_edit.h
    src/ui/dialogs/command_palette_dialog.cpp
    src/ui/dialogs/command_palette_dialog.h
    src/ui/dialogs/settings_dialog.cpp
    src/ui/dialogs/settings_dialog.h
    src/ui/dialogs/python_tools_dialog.cpp
    src/ui/dialogs/python_tools_dialog.h
    src/ui/editor/editor_workspace_widget.cpp
    src/ui/editor/editor_workspace_widget.h
    src/ui/editor/model_document_widget.cpp
    src/ui/editor/model_document_widget.h
    src/ui/editor/model_summary_widget.cpp
    src/ui/editor/model_summary_widget.h
    src/ui/editor/occt_model_view_widget.cpp
    src/ui/editor/occt_model_view_widget.h
    src/ui/editor/editor_placeholder_widget.cpp
    src/ui/editor/editor_placeholder_widget.h
    src/ui/editor/script_editor_widget.cpp
    src/ui/editor/script_editor_widget.h
    src/ui/mainwindow/main_window.cpp
    src/ui/mainwindow/main_window.h
    src/ui/panels/properties/model_properties_panel.cpp
    src/ui/panels/properties/model_properties_panel.h
    src/ui/panels/files/file_browser_panel.cpp
    src/ui/panels/files/file_browser_panel.h
    src/ui/panels/plugins/plugin_manager_panel.cpp
    src/ui/panels/plugins/plugin_manager_panel.h
    src/ui/panels/python/python_console_widget.cpp
    src/ui/panels/python/python_console_widget.h
    ${PYRAQT_RESOURCE_SOURCES}
    ${PYRAQT_TRANSLATION_SOURCES}
)

target_link_libraries(pyraqt
    PRIVATE
        pyraqt_core
        PyraQt::QtCore
        PyraQt::QtGui
        PyraQt::QtWidgets
        PyraQt::QtSvg
        PyraQt::QtOpenGLWidgets
)

if(PYRAQT_HAS_QSCINTILLA)
    target_link_libraries(pyraqt PRIVATE QScintilla::QScintilla)
    target_compile_definitions(pyraqt PRIVATE PYRAQT_HAS_QSCINTILLA=1)
endif()

target_compile_definitions(pyraqt_core
    PUBLIC
        PYRAQT_HAS_QSCINTILLA=$<BOOL:${PYRAQT_HAS_QSCINTILLA}>
        PYRAQT_HAS_OCCT=$<BOOL:${PYRAQT_HAS_OCCT}>
)

pyraqt_set_project_warnings(pyraqt)

add_custom_command(TARGET pyraqt POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:pyraqt>/translations"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:pyraqt>/plugins/python"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${PYRAQT_TRANSLATION_SOURCES}
        "$<TARGET_FILE_DIR:pyraqt>/translations"
)

add_custom_command(TARGET pyraqt POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/plugins"
        "$<TARGET_FILE_DIR:pyraqt>/plugins/python"
)

function(pyraqt_add_cpp_plugin target_name)
    add_library(${target_name} MODULE ${ARGN})
    target_link_libraries(${target_name} PRIVATE pyraqt_core PyraQt::QtCore PyraQt::QtGui PyraQt::QtWidgets)
    target_include_directories(${target_name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    set_target_properties(${target_name} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/cpp"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/cpp"
        PREFIX ""
    )
    pyraqt_set_project_warnings(${target_name})
endfunction()

if(PYRAQT_ENABLE_PLUGINS)
    pyraqt_add_cpp_plugin(pyraqt_sample_cpp_plugin
        src/plugins/sample_cpp_plugin/sample_cpp_plugin.cpp
        src/plugins/sample_cpp_plugin/sample_cpp_plugin.h
    )
    add_custom_command(TARGET pyraqt POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:pyraqt>/plugins/cpp"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:pyraqt_sample_cpp_plugin>"
            "$<TARGET_FILE_DIR:pyraqt>/plugins/cpp"
    )
endif()

if(APPLE)
    set_target_properties(pyraqt PROPERTIES MACOSX_BUNDLE TRUE)
endif()

include(CTest)
if(BUILD_TESTING AND PYRAQT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

include(Packaging)
