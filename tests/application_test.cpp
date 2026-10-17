#include "appconfig.h"
#include "application_harness.h"
#include "clipboard.h"
#include "documentsession.h"

#include <QCoreApplication>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QElapsedTimer>
#include <QFile>
#include <QMimeData>
#include <QProcess>
#include <QProcessEnvironment>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QTest>

#include <gtest/gtest.h>
#include <memory>

TEST(ApplicationZoom, KeyboardShortcutsStepAndResetZoom) {
  ApplicationHarness harness;
  const auto& root = harness.root;
  ASSERT_NE(root, nullptr) << harness.error.toStdString();
  auto* window = qobject_cast<QQuickWindow*>(root.get());
  ASSERT_NE(window, nullptr);
  auto* reader = root->findChild<QQuickItem*>(QStringLiteral("readerPane"));
  ASSERT_NE(reader, nullptr);
  QTest::keyClick(window, Qt::Key_0, Qt::ControlModifier);
  EXPECT_DOUBLE_EQ(reader->property("zoom").toDouble(), 1.0);
  QTest::keyClick(window, Qt::Key_Equal, Qt::ControlModifier);
  EXPECT_DOUBLE_EQ(reader->property("zoom").toDouble(), 1.1);
  QTest::keyClick(window, Qt::Key_Plus, Qt::ControlModifier);
  EXPECT_DOUBLE_EQ(reader->property("zoom").toDouble(), 1.25);
  QTest::keyClick(window, Qt::Key_Minus, Qt::ControlModifier);
  QTest::keyClick(window, Qt::Key_Minus, Qt::ControlModifier);
  EXPECT_DOUBLE_EQ(reader->property("zoom").toDouble(), 1.0);
  for (int i = 0; i < 20; ++i) {
    QTest::keyClick(window, Qt::Key_Minus, Qt::ControlModifier);
  }
  EXPECT_DOUBLE_EQ(reader->property("zoom").toDouble(), 0.5);
  QTest::keyClick(window, Qt::Key_0, Qt::ControlModifier);
  EXPECT_DOUBLE_EQ(reader->property("zoom").toDouble(), 1.0);
}

TEST(ApplicationWidth, ShortcutCyclesColumnWidthPresets) {
  ApplicationHarness harness;
  const auto& root = harness.root;
  ASSERT_NE(root, nullptr) << harness.error.toStdString();
  auto* window = qobject_cast<QQuickWindow*>(root.get());
  ASSERT_NE(window, nullptr);
  auto* reader = root->findChild<QQuickItem*>(QStringLiteral("readerPane"));
  ASSERT_NE(reader, nullptr);
  const auto width = [reader] { return reader->property("maxColumnWidth").toDouble(); };
  const double start = width();
  QTest::keyClick(window, Qt::Key_W, Qt::ControlModifier | Qt::ShiftModifier);
  const double second = width();
  QTest::keyClick(window, Qt::Key_W, Qt::ControlModifier | Qt::ShiftModifier);
  const double third = width();
  QTest::keyClick(window, Qt::Key_W, Qt::ControlModifier | Qt::ShiftModifier);
  EXPECT_NE(start, second);
  EXPECT_NE(second, third);
  EXPECT_NE(start, third);
  EXPECT_DOUBLE_EQ(width(), start);
}

TEST(ApplicationSettings, ShortcutOpensModalDialogThatEditsConfigAndEscapeCloses) {
  ApplicationHarness harness;
  const auto& root = harness.root;
  ASSERT_NE(root, nullptr) << harness.error.toStdString();
  auto* window = qobject_cast<QQuickWindow*>(root.get());
  ASSERT_NE(window, nullptr);
  auto* reader = root->findChild<QQuickItem*>(QStringLiteral("readerPane"));
  auto* dialog = root->findChild<QObject*>(QStringLiteral("settingsDialog"));
  ASSERT_NE(reader, nullptr);
  ASSERT_NE(dialog, nullptr);
  EXPECT_FALSE(dialog->property("visible").toBool());
  QTest::keyClick(window, Qt::Key_Comma, Qt::ControlModifier);
  QCoreApplication::processEvents(QEventLoop::AllEvents, 300);
  EXPECT_TRUE(dialog->property("visible").toBool());
  EXPECT_TRUE(dialog->property("modal").toBool());

  auto* width_box = dialog->findChild<QObject*>(QStringLiteral("widthBox"));
  ASSERT_NE(width_box, nullptr);
  ASSERT_TRUE(QMetaObject::invokeMethod(width_box, "activated", Q_ARG(int, 2)));
  EXPECT_DOUBLE_EQ(reader->property("maxColumnWidth").toDouble(), 1000.0);

  auto* zoom_slider = dialog->findChild<QObject*>(QStringLiteral("zoomSlider"));
  ASSERT_NE(zoom_slider, nullptr);
  ASSERT_TRUE(zoom_slider->setProperty("value", 1.5));
  ASSERT_TRUE(QMetaObject::invokeMethod(zoom_slider, "moved"));
  EXPECT_DOUBLE_EQ(reader->property("zoom").toDouble(), 1.5);

  auto* theme_box = dialog->findChild<QObject*>(QStringLiteral("themeBox"));
  ASSERT_NE(theme_box, nullptr);
  ASSERT_TRUE(QMetaObject::invokeMethod(theme_box, "activated", Q_ARG(int, 2)));
  EXPECT_TRUE(reader->property("dark").toBool());
  ASSERT_TRUE(QMetaObject::invokeMethod(theme_box, "activated", Q_ARG(int, 1)));
  EXPECT_FALSE(reader->property("dark").toBool());

  auto* spacing_slider = dialog->findChild<QObject*>(QStringLiteral("spacingSlider"));
  ASSERT_NE(spacing_slider, nullptr);
  ASSERT_TRUE(spacing_slider->setProperty("value", 1.5));
  ASSERT_TRUE(QMetaObject::invokeMethod(spacing_slider, "moved"));
  EXPECT_DOUBLE_EQ(reader->property("blockSpacingScale").toDouble(), 1.5);

  auto* underline_switch = dialog->findChild<QObject*>(QStringLiteral("underlineSwitch"));
  ASSERT_NE(underline_switch, nullptr);
  ASSERT_TRUE(underline_switch->setProperty("checked", false));
  ASSERT_TRUE(QMetaObject::invokeMethod(underline_switch, "toggled"));
  EXPECT_FALSE(reader->property("linkUnderline").toBool());

  QTest::keyClick(window, Qt::Key_Escape);
  QCoreApplication::processEvents(QEventLoop::AllEvents, 300);
  EXPECT_FALSE(dialog->property("visible").toBool());
}

TEST(ApplicationIsolation, EachHarnessStartsWithDefaults) {
  for (int i = 0; i < 2; ++i) {
    ApplicationHarness harness;
    ASSERT_NE(harness.root, nullptr) << harness.error.toStdString();
    auto* config = harness.root->findChild<AppConfig*>();
    ASSERT_NE(config, nullptr);
    EXPECT_DOUBLE_EQ(config->zoom(), 1.0);
    EXPECT_EQ(config->widthPreset(), AppConfig::Comfortable);
    config->setZoom(2.0);
    ASSERT_TRUE(config->flush());
  }
}

TEST(ApplicationIsolation, ShortcutSubprocessPreservesUserFallback) {
  QTemporaryDir home;
  ASSERT_TRUE(home.isValid());
  ASSERT_TRUE(QDir().mkpath(home.filePath(".config")));
  const QString path = home.filePath(".config/md-viewer.toml");
  const QByteArray sentinel = "[reader]\nzoom = 1.75\ncolumn_width = \"narrow\"\n";
  QFile file(path);
  ASSERT_TRUE(file.open(QIODevice::WriteOnly));
  ASSERT_EQ(file.write(sentinel), sentinel.size());
  file.close();
  QProcess process;
  auto environment = QProcessEnvironment::systemEnvironment();
  environment.insert("HOME", home.path());
  environment.insert("XDG_CONFIG_HOME", home.filePath("xdg"));
  process.setProcessEnvironment(environment);
  process.start(QCoreApplication::applicationFilePath(),
                {"--gtest_filter=ApplicationZoom.KeyboardShortcutsStepAndResetZoom"});
  ASSERT_TRUE(process.waitForFinished(10000));
  EXPECT_EQ(process.exitCode(), 0) << process.readAllStandardOutput().toStdString();
  ASSERT_TRUE(file.open(QIODevice::ReadOnly));
  EXPECT_EQ(file.readAll(), sentinel);
}

TEST(ApplicationWidth, NamedPresetsSelectExpectedWidths) {
  ApplicationHarness harness;
  ASSERT_NE(harness.root, nullptr) << harness.error.toStdString();
  auto* config = harness.root->findChild<AppConfig*>();
  auto* reader = harness.root->findChild<QQuickItem*>(QStringLiteral("readerPane"));
  ASSERT_NE(config, nullptr);
  ASSERT_NE(reader, nullptr);
  for (const auto& [preset, width] :
       {std::pair{AppConfig::Narrow, 600.0}, {AppConfig::Comfortable, 800.0}, {AppConfig::Wide, 1000.0}}) {
    config->setWidthPreset(preset);
    EXPECT_DOUBLE_EQ(reader->property("maxColumnWidth").toDouble(), width);
  }
}
