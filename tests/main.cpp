#include <QGuiApplication>

#include <gtest/gtest.h>

int main(int argc, char* argv[]) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  qputenv("QT_QUICK_BACKEND", "software");
  qputenv("QT_QUICK_CONTROLS_STYLE", "Fusion");
  qputenv("QT_QPA_PLATFORMTHEME", "");
  const QGuiApplication app(argc, argv);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
