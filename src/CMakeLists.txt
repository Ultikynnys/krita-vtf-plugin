set(kritavtfcodec_SOURCES kis_vtf_codec.cpp)

set(kritavtfimport_SOURCES
    kis_vtf_import.cpp
    ${kritavtfcodec_SOURCES}
)
kis_add_library(kritavtfimport MODULE ${kritavtfimport_SOURCES})
target_link_libraries(kritavtfimport kritaui)
install(TARGETS kritavtfimport DESTINATION ${KRITA_PLUGIN_INSTALL_DIR})

set(kritavtfexport_SOURCES
    kis_vtf_export.cpp
    ${kritavtfcodec_SOURCES}
)
kis_add_library(kritavtfexport MODULE ${kritavtfexport_SOURCES})
target_link_libraries(kritavtfexport kritaui kritaimpex)
install(TARGETS kritavtfexport DESTINATION ${KRITA_PLUGIN_INSTALL_DIR})

install(FILES vtf.xml DESTINATION ${KDE_INSTALL_MIMEDIR}/packages)
