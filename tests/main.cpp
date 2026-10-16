#include <QGuiApplication>
#include <QTemporaryDir>

#include <gtest/gtest.h>

int main(int argc, char* argv[]) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  qputenv("QT_QUICK_BACKEND", "software");
  qputenv("QT_QUICK_CONTROLS_STYLE", "Fusion");
  qputenv("QT_QPA_PLATFORMTHEME", "");
  // Keep the application configuration away from the real user home.
  const QTemporaryDir config_home;
  qputenv("XDG_CONFIG_HOME", config_home.path().toUtf8());
  const QGuiApplication app(argc, argv);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
