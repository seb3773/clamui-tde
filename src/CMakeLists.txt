include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/libs/json
  ${CMAKE_SOURCE_DIR}/libs/process
  ${CMAKE_SOURCE_DIR}/libs/tqt-charts
  ${CMAKE_SOURCE_DIR}/libs/concurrent
  ${CMAKE_SOURCE_DIR}/libs/http
  ${CMAKE_SOURCE_DIR}/libs/serializer
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)

##### tdeclamui (executable) ###############################

# --- tde-extended libs (compiled in) ---
set( LIBS_DIR ${CMAKE_SOURCE_DIR}/libs )

set( libs_SRCS
  # JSON
  ${LIBS_DIR}/json/parson.c
  ${LIBS_DIR}/json/tqtjson.cpp
  # Process
  ${LIBS_DIR}/process/tqtprocess.cpp
  # Charts
  ${LIBS_DIR}/tqt-charts/tqtabstractchartwidget.cpp
  ${LIBS_DIR}/tqt-charts/tqtbarchartwidget.cpp
  ${LIBS_DIR}/tqt-charts/tqtpiecechartwidget.cpp
  ${LIBS_DIR}/tqt-charts/tqtstackedbarchartwidget.cpp
  # Concurrent
  ${LIBS_DIR}/concurrent/tqtconcurrent.cpp
  # HTTP network
  ${LIBS_DIR}/http/tqtnetwork.cpp
  # Serializer
  ${LIBS_DIR}/serializer/tqtabstractserializer.cpp
  ${LIBS_DIR}/serializer/tqtstringserializer.cpp
)

# --- Core business logic ---
set( core_SRCS
  core/clamav_detection.cpp
  core/scanner.cpp
  core/updater.cpp
  core/settings_manager.cpp
  core/log_manager.cpp
  core/quarantine_manager.cpp
  core/threat_classifier.cpp
  core/statistics.cpp
  core/system_audit.cpp
  core/profiles.cpp
  core/scheduler.cpp
  core/headless_scanner.cpp
  core/virustotal.cpp
  core/notification_manager.cpp
  core/unofficial_sigs.cpp
)

# --- UI layer ---
set( ui_SRCS
  ui/mainwindow.cpp
  ui/sidebar.cpp
  ui/scan_view.cpp
  ui/scan_results_dialog.cpp
  ui/profile_dialogs.cpp
  ui/update_view.cpp
  ui/logs_view.cpp
  ui/components_view.cpp
  ui/quarantine_view.cpp
  ui/statistics_view.cpp
  ui/audit_view.cpp
  ui/about_dialog.cpp
  ui/preferences/prefs_dialog.cpp
  ui/preferences/behavior_page.cpp
  ui/preferences/database_page.cpp
  ui/preferences/scanner_page.cpp
  ui/preferences/exclusions_page.cpp
  ui/preferences/scheduled_page.cpp
  ui/preferences/virustotal_page.cpp
  ui/preferences/debug_page.cpp
  tray.cpp
  clamui_app.cpp
)

# --- All sources ---
set( tdeclamui_SRCS
  main.cpp
  ${libs_SRCS}
  ${core_SRCS}
  ${ui_SRCS}
)

# --- Embedded icons generation ---
file(GLOB ICON_PNGS "${CMAKE_SOURCE_DIR}/icons/*.png")
set(EMBEDDED_ICONS_HDR "${CMAKE_SOURCE_DIR}/src/embedded_icons.h")
set(GEN_EMBEDDED_ICONS_SH "${CMAKE_SOURCE_DIR}/scripts/gen_embedded_icons.sh")

add_custom_command(
    OUTPUT "${EMBEDDED_ICONS_HDR}"
    COMMAND /usr/bin/env bash "${GEN_EMBEDDED_ICONS_SH}"
    DEPENDS ${ICON_PNGS} "${GEN_EMBEDDED_ICONS_SH}"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    VERBATIM
)

add_custom_target(embedded_icons DEPENDS "${EMBEDDED_ICONS_HDR}")

# --- MOC generation ---
tde_automoc( ${tdeclamui_SRCS} )

# Disable tdelfeditor to prevent harmless but annoying errors about ELF resources and missing icons
set(_TDELFEDITOR_EXECUTABLE ${TDELFEDITOR_EXECUTABLE})
set(TDELFEDITOR_EXECUTABLE "")

tde_add_executable( tdeclamui AUTOMOC
  SOURCES ${tdeclamui_SRCS}
  LINK
    tdecore-shared tdeui-shared tdeio-shared
    ${SQLITE3_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${LIBNOTIFY_LIBRARIES}
    ${LIBCURL_LIBRARIES}
    pthread
  DESTINATION ${BIN_INSTALL_DIR}
)

# Restore tdelfeditor
set(TDELFEDITOR_EXECUTABLE ${_TDELFEDITOR_EXECUTABLE})

add_dependencies(tdeclamui embedded_icons)

if (SSSTRIP_EXECUTABLE)
    add_custom_command(TARGET tdeclamui POST_BUILD
        COMMAND ${SSSTRIP_EXECUTABLE} $<TARGET_FILE:tdeclamui>
        VERBATIM
    )
endif()
