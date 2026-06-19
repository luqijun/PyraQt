find_path(QSCINTILLA_INCLUDE_DIR
    NAMES Qsci/qsciscintilla.h
    PATH_SUFFIXES qt5 qt6
)

find_library(QSCINTILLA_LIBRARY
    NAMES qscintilla2_qt5 qscintilla2_qt6
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(QScintilla
    REQUIRED_VARS QSCINTILLA_INCLUDE_DIR QSCINTILLA_LIBRARY
)

if(QScintilla_FOUND AND NOT TARGET QScintilla::QScintilla)
    add_library(QScintilla::QScintilla UNKNOWN IMPORTED)
    set_target_properties(QScintilla::QScintilla PROPERTIES
        IMPORTED_LOCATION "${QSCINTILLA_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${QSCINTILLA_INCLUDE_DIR}"
    )
endif()
