#pragma once

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QTemporaryDir>

#include <memory>

// Member order keeps the isolated directory alive until the application and engine are destroyed.
struct ApplicationHarness {
  QTemporaryDir config_home;
  QQmlEngine engine;
  std::unique_ptr<QObject> root;
  QString error;

  explicit ApplicationHarness(const QByteArray& initial_config = {}) {
    if (!config_home.isValid()) {
      error = QStringLiteral("Cannot create test configuration directory");
      return;
    }
    qputenv("XDG_CONFIG_HOME", config_home.path().toUtf8());
    if (!initial_config.isEmpty()) {
      QDir().mkpath(config_home.filePath("md-viewer"));
      QFile file(config_home.filePath("md-viewer/config.toml"));
      if (!file.open(QIODevice::WriteOnly) || file.write(initial_config) != initial_config.size()) {
        error = QStringLiteral("Cannot write initial test configuration");
        return;
      }
    }
    engine.addImportPath(QString::fromUtf8(MD_VIEWER_QMARKDOWN_IMPORT_PATH));
    QQmlComponent component(&engine, QUrl::fromLocalFile(QString::fromUtf8(MD_VIEWER_MAIN_QML)));
    root.reset(component.create());
    error = component.errorString();
  }
};

// Qt's QTRY_* macros return silently outside a QTest function; pair this with ASSERT_TRUE instead.
template <typename Condition>
bool waitFor(const Condition& condition, int timeout_ms = 5000) {
  QElapsedTimer timer;
  timer.start();
  while (!condition()) {
    if (timer.elapsed() > timeout_ms) {
      return false;
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
  }
  return true;
}
