cmake_minimum_required(VERSION 3.25)
project(md-viewer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(GNUInstallDirs)
include(CTest)

# Dependency acquisition is explicit; configuration never accesses the network.
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Concurrent)
find_package(tomlplusplus 3 CONFIG REQUIRED)
find_package(QMarkdown 1.13 CONFIG REQUIRED)
if(QMarkdown_VERSION VERSION_GREATER_EQUAL 2.0)
  message(FATAL_ERROR "QMarkdown 2.x requires a reviewed migration; use 1.13 or a compatible 1.x release.")
endif()
get_target_property(qmarkdown_type QMarkdown::QMarkdown TYPE)
if(NOT qmarkdown_type STREQUAL "SHARED_LIBRARY")
  message(FATAL_ERROR "This scaffold supports shared QMarkdown packages. Static deployment is not configured.")
endif()
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/qmarkdown-library-dir.txt"
  CONTENT "$<TARGET_FILE_DIR:QMarkdown::QMarkdown>\n")
list(APPEND QML_IMPORT_PATH "${QMarkdown_QML_IMPORT_PATH}")
set(QML_IMPORT_PATH "${QML_IMPORT_PATH}" CACHE STRING "Additional QML module roots" FORCE)
qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(mdv src/main.cpp)
set_source_files_properties(qml/Main.qml PROPERTIES QT_RESOURCE_ALIAS Main.qml)
set_source_files_properties(qml/ReaderPane.qml PROPERTIES QT_RESOURCE_ALIAS ReaderPane.qml)
set_source_files_properties(qml/TaskIndicator.qml PROPERTIES QT_RESOURCE_ALIAS TaskIndicator.qml)
set_source_files_properties(qml/CodeHeader.qml PROPERTIES QT_RESOURCE_ALIAS CodeHeader.qml)
set_source_files_properties(qml/CodeScrollBar.qml PROPERTIES QT_RESOURCE_ALIAS CodeScrollBar.qml)
set_source_files_properties(qml/SettingsDialog.qml PROPERTIES QT_RESOURCE_ALIAS SettingsDialog.qml)
qt_add_qml_module(mdv
  URI MdViewer
  VERSION 1.0
  # Shared providers are loaded through the QML engine, not static plugin imports.
  NO_IMPORT_SCAN
  QML_FILES qml/Main.qml qml/ReaderPane.qml qml/SettingsDialog.qml qml/CodeHeader.qml qml/CodeScrollBar.qml qml/TaskIndicator.qml
  SOURCES src/appconfig.h src/appconfig.cpp src/clipboard.h src/clipboard.cpp src/documentsession.h
    src/documentsession.cpp src/navigationcontroller.h src/navigationcontroller.cpp
  IMPORT_PATH "${QMarkdown_QML_IMPORT_PATH}"
)
target_link_libraries(mdv PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Concurrent QMarkdown::QMarkdown
  tomlplusplus::tomlplusplus)
target_include_directories(mdv PRIVATE src)
# The explicitly selected installed provider is also used by local runs.
# Distributors should use their final system prefix and deploy its QML module.
target_compile_definitions(mdv PRIVATE MD_VIEWER_QMARKDOWN_IMPORT_PATH="${QMarkdown_QML_IMPORT_PATH}")
target_compile_options(mdv PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
qt_add_resources(mdv "application-icon" PREFIX "/" FILES assets/mdv.svg assets/copy.svg)
set_target_properties(mdv PROPERTIES INSTALL_RPATH "$<TARGET_FILE_DIR:QMarkdown::QMarkdown>")
install(FILES assets/mdv.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES assets/mdv.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
configure_file(cmake/RefreshDesktopCaches.cmake.in RefreshDesktopCaches.cmake @ONLY)
install(TARGETS mdv RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/RefreshDesktopCaches.cmake")

if(BUILD_TESTING)
  find_package(Qt6 6.8 REQUIRED COMPONENTS Test)
  find_package(GTest 1.12 CONFIG REQUIRED)
  add_executable(reader-tests tests/main.cpp tests/navigation_test.cpp tests/reader_test.cpp tests/documentsession_test.cpp tests/drop_test.cpp tests/application_test.cpp tests/appconfig_test.cpp
    src/appconfig.h src/appconfig.cpp src/documentsession.h src/documentsession.cpp src/navigationcontroller.h src/navigationcontroller.cpp src/clipboard.h src/clipboard.cpp)
  qt_add_resources(reader-tests "application-qml" PREFIX "/qt/qml/MdViewer"
    FILES qml/Main.qml qml/ReaderPane.qml qml/SettingsDialog.qml qml/CodeHeader.qml qml/CodeScrollBar.qml qml/TaskIndicator.qml)
  qt_add_resources(reader-tests "copy-icon" PREFIX "/" FILES assets/copy.svg)
  target_link_libraries(reader-tests PRIVATE GTest::gtest Qt6::Quick Qt6::QuickControls2 Qt6::Concurrent Qt6::Test QMarkdown::QMarkdown
    tomlplusplus::tomlplusplus)
  target_include_directories(reader-tests PRIVATE src)
  target_compile_definitions(reader-tests PRIVATE
    MD_VIEWER_MAIN_QML="${CMAKE_CURRENT_SOURCE_DIR}/qml/Main.qml"
    MD_VIEWER_READER_QML="${CMAKE_CURRENT_SOURCE_DIR}/qml/ReaderPane.qml"
    MD_VIEWER_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/tests/data"
    MD_VIEWER_QMARKDOWN_IMPORT_PATH="${QMarkdown_QML_IMPORT_PATH}")
  include(GoogleTest)
  gtest_discover_tests(reader-tests DISCOVERY_MODE PRE_TEST
    PROPERTIES TIMEOUT 15 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QT_QUICK_CONTROLS_STYLE=Fusion;QT_QPA_PLATFORMTHEME=")
  add_test(NAME application-startup COMMAND mdv --smoke-test)
  set_tests_properties(application-startup PROPERTIES TIMEOUT 15
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QT_QUICK_CONTROLS_STYLE=Fusion;QT_QPA_PLATFORMTHEME=")
endif()
