include(GNUInstallDirs)

install(TARGETS pyraqt
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION .
)

install(DIRECTORY resources/themes
    DESTINATION ${CMAKE_INSTALL_DATADIR}/pyraqt/resources
)

install(FILES ${PYRAQT_TRANSLATION_SOURCES}
    DESTINATION ${CMAKE_INSTALL_BINDIR}/translations
)

install(DIRECTORY scripts
    DESTINATION ${CMAKE_INSTALL_DATADIR}/pyraqt
)

install(DIRECTORY plugins/
    DESTINATION ${CMAKE_INSTALL_BINDIR}/plugins/python
)

if(TARGET pyraqt_sample_cpp_plugin)
    install(TARGETS pyraqt_sample_cpp_plugin
        LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}/plugins/cpp
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/plugins/cpp
    )
endif()

set(CPACK_PACKAGE_NAME "PyraQt")
set(CPACK_PACKAGE_VENDOR "PyraQt")
set(CPACK_PACKAGE_CONTACT "maintainers@pyraqt.local")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Modern Qt desktop application template framework")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_FILE_NAME "PyraQt-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}")
set(CPACK_SOURCE_IGNORE_FILES
    "/build/"
    "/.git/"
    "/python-from-qgis-project/"
)

if(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "DEB;TGZ;ZIP")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "PyraQt Maintainers")
    set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5core5a, libqt5gui5, libqt5widgets5, python3")
else()
    set(CPACK_GENERATOR "ZIP")
endif()

include(CPack)
