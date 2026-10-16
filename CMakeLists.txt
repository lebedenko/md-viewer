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
find_package(QMarkdown 1.10 CONFIG REQUIRED)
if(QMarkdown_VERSION VERSION_GREATER_EQUAL 2.0)
  message(FATAL_ERROR "QMarkdown 2.x requires a reviewed migration; use 1.10 or a compatible 1.x release.")
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

qt_add_executable(md-viewer src/main.cpp)
set_source_files_properties(qml/Main.qml PROPERTIES QT_RESOURCE_ALIAS Main.qml)
set_source_files_properties(qml/ReaderPane.qml PROPERTIES QT_RESOURCE_ALIAS ReaderPane.qml)
qt_add_qml_module(md-viewer
  URI MdViewer
  VERSION 1.0
  # Shared providers are loaded through the QML engine, not static plugin imports.
  NO_IMPORT_SCAN
  QML_FILES qml/Main.qml qml/ReaderPane.qml
  SOURCES src/clipboard.h src/clipboard.cpp src/documentsession.h src/documentsession.cpp
  IMPORT_PATH "${QMarkdown_QML_IMPORT_PATH}"
)
target_link_libraries(md-viewer PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Concurrent QMarkdown::QMarkdown)
target_include_directories(md-viewer PRIVATE src)
# The explicitly selected installed provider is also used by local runs.
# Distributors should use their final system prefix and deploy its QML module.
target_compile_definitions(md-viewer PRIVATE MD_VIEWER_QMARKDOWN_IMPORT_PATH="${QMarkdown_QML_IMPORT_PATH}")
target_compile_options(md-viewer PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
install(TARGETS md-viewer RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(BUILD_TESTING)
  find_package(GTest 1.12 CONFIG REQUIRED)
  add_executable(reader-tests tests/main.cpp tests/reader_test.cpp tests/documentsession_test.cpp tests/drop_test.cpp
    src/documentsession.h src/documentsession.cpp src/clipboard.h src/clipboard.cpp)
  qt_add_resources(reader-tests "application-qml" PREFIX "/qt/qml/MdViewer"
    FILES qml/Main.qml qml/ReaderPane.qml)
  target_link_libraries(reader-tests PRIVATE GTest::gtest Qt6::Quick Qt6::QuickControls2 Qt6::Concurrent QMarkdown::QMarkdown)
  target_include_directories(reader-tests PRIVATE src)
  target_compile_definitions(reader-tests PRIVATE
    MD_VIEWER_MAIN_QML="${CMAKE_CURRENT_SOURCE_DIR}/qml/Main.qml"
    MD_VIEWER_READER_QML="${CMAKE_CURRENT_SOURCE_DIR}/qml/ReaderPane.qml"
    MD_VIEWER_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/tests/data"
    MD_VIEWER_QMARKDOWN_IMPORT_PATH="${QMarkdown_QML_IMPORT_PATH}")
  include(GoogleTest)
  gtest_discover_tests(reader-tests DISCOVERY_MODE PRE_TEST
    PROPERTIES TIMEOUT 15 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QT_QUICK_CONTROLS_STYLE=Fusion;QT_QPA_PLATFORMTHEME=")
  add_test(NAME application-startup COMMAND md-viewer --smoke-test)
  set_tests_properties(application-startup PROPERTIES TIMEOUT 15
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QT_QUICK_CONTROLS_STYLE=Fusion;QT_QPA_PLATFORMTHEME=")
endif()
