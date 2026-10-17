#include "appconfig.h"
#include "clipboard.h"
#include "documentsession.h"

#include <QGuiApplication>
#include <QQmlEngine>
#include <QTemporaryDir>

#include <gtest/gtest.h>

int main(int argc, char* argv[]) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  qputenv("QT_QUICK_BACKEND", "software");
  qputenv("QT_QUICK_CONTROLS_STYLE", "Fusion");
  qputenv("QT_QPA_PLATFORMTHEME", "");
  // Keep the application configuration away from the real user home.
  const QTemporaryDir config_home;
  if (!config_home.isValid()) {
    return 1;
  }
  qputenv("HOME", config_home.path().toUtf8());
  qputenv("XDG_CONFIG_HOME", config_home.path().toUtf8());
  const QGuiApplication app(argc, argv);
  qmlRegisterType<DocumentSession>("MdViewer", 1, 0, "DocumentSession");
  qmlRegisterType<AppConfig>("MdViewer", 1, 0, "AppConfig");
  qmlRegisterSingletonType<Clipboard>("MdViewer", 1, 0, "Clipboard",
                                      [](QQmlEngine*, QJSEngine*) -> QObject* { return new Clipboard; });
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
