#include "appconfig.h"
#include "application_harness.h"
#include "clipboard.h"
#include "documentsession.h"
#include "navigationcontroller.h"

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

TEST(ApplicationReaderFeatures, FindOutlineSettingsAndSelectionShortcuts) {
  ApplicationHarness harness;
  ASSERT_NE(harness.root, nullptr) << harness.error.toStdString();
  auto* window = qobject_cast<QQuickWindow*>(harness.root.get());
  auto* pane = harness.root->findChild<QQuickItem*>(QStringLiteral("readerPane"));
  auto* document = harness.root->findChild<QQuickItem*>(QStringLiteral("markdownDocument"));
  auto* config = harness.root->findChild<AppConfig*>();
  ASSERT_NE(window, nullptr);
  ASSERT_NE(pane, nullptr);
  ASSERT_NE(document, nullptr);
  ASSERT_NE(config, nullptr);
  QTest::qWait(30);
  EXPECT_FALSE(pane->property("outlineVisible").toBool());
  QTest::keyClick(window, Qt::Key_O, Qt::ControlModifier | Qt::ShiftModifier);
  EXPECT_TRUE(pane->property("outlineVisible").toBool());
  QTest::keyClick(window, Qt::Key_F, Qt::ControlModifier);
  EXPECT_TRUE(document->property("searchActive").toBool());
  QTest::keyClick(window, Qt::Key_M);
  EXPECT_GT(document->property("searchResultCount").toInt(), 0);
  QTest::keyClick(window, Qt::Key_Escape);
  EXPECT_FALSE(document->property("searchActive").toBool());
  QTest::keyClick(window, Qt::Key_F, Qt::ControlModifier);
  EXPECT_EQ(document->property("searchQuery").toString(), "m");
  QTest::keyClick(window, Qt::Key_Escape);
  config->setSelectionEnabled(false);
  EXPECT_FALSE(document->property("selectionEnabled").toBool());
  QTest::keyClick(window, Qt::Key_A, Qt::ControlModifier);
  EXPECT_TRUE(config->selectionEnabled());
  EXPECT_FALSE(document->property("selectedText").toString().isEmpty());
  config->setCodeWrapping(true);
  config->setLineNumbers(false);
  config->setHeightLimiting(false);
  EXPECT_TRUE(document->property("codeBlockWrapEnabled").toBool());
  EXPECT_FALSE(document->property("codeBlockLineNumbersVisible").toBool());
  EXPECT_FALSE(document->property("codeBlockHeightLimitEnabled").toBool());
}

TEST(ApplicationReaderFeatures, NavigationScrollClampsAndHistoryRestoresPosition) {
  ApplicationHarness harness;
  ASSERT_NE(harness.root, nullptr) << harness.error.toStdString();
  auto* nav = harness.root->findChild<NavigationController*>();
  auto* document = harness.root->findChild<QQuickItem*>(QStringLiteral("markdownDocument"));
  auto* viewport = harness.root->findChild<QQuickItem*>(QStringLiteral("readerViewport"));
  auto* pane = harness.root->findChild<QQuickItem*>(QStringLiteral("readerPane"));
  ASSERT_NE(nav, nullptr);
  ASSERT_NE(document, nullptr);
  ASSERT_NE(viewport, nullptr);
  ASSERT_NE(pane, nullptr);
  QTemporaryDir dir;
  QString source = "# Start\n\n[web](https://example.org)\n\n";
  for (int i = 0; i < 80; ++i) {
    source += "Paragraph with enough text to scroll.\n\n";
  }
  source += "## Finish\n\nLast paragraph.\n";
  QFile file(dir.filePath("a.md"));
  ASSERT_TRUE(file.open(QIODevice::WriteOnly));
  file.write(source.toUtf8());
  file.close();
  QFile other(dir.filePath("b.md"));
  ASSERT_TRUE(other.open(QIODevice::WriteOnly));
  other.write("# Other\n");
  other.close();
  const auto settle = [&nav] {
    for (int i = 0; i < 500 && nav->session()->state() == DocumentSession::State::Loading; ++i) {
      QTest::qWait(10);
    }
    QTest::qWait(500);
  };
  nav->openPath(file.fileName());
  settle();
  ASSERT_EQ(nav->session()->state(), DocumentSession::State::Ready);
  ASSERT_TRUE(QMetaObject::invokeMethod(document, "selectionScrollRequested", Q_ARG(qreal, 100000.0)));
  const double range = viewport->property("contentHeight").toDouble() - viewport->height();
  ASSERT_GT(range, 0);
  EXPECT_DOUBLE_EQ(viewport->property("contentY").toDouble(), range);
  ASSERT_TRUE(QMetaObject::invokeMethod(document, "selectionScrollRequested", Q_ARG(qreal, -100000.0)));
  EXPECT_DOUBLE_EQ(viewport->property("contentY").toDouble(), 0);
  viewport->setProperty("contentY", range * 0.45);
  nav->activate("b.md");
  settle();
  EXPECT_DOUBLE_EQ(viewport->property("contentY").toDouble(), 0);
  nav->back();
  settle();
  EXPECT_NEAR(viewport->property("contentY").toDouble(), range * 0.45, 2);
  const int revision = nav->session()->revision();
  ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
  file.write((source + "\nReloaded paragraph.\n").toUtf8());
  file.close();
  for (int i = 0; i < 500 && nav->session()->revision() == revision; ++i) {
    QTest::qWait(10);
  }
  settle();
  ASSERT_GT(nav->session()->revision(), revision);
  const double newRange = viewport->property("contentHeight").toDouble() - viewport->height();
  EXPECT_NEAR(viewport->property("contentY").toDouble() / newRange, 0.45, 0.03);
  auto* window = qobject_cast<QQuickWindow*>(harness.root.get());
  ASSERT_NE(window, nullptr);
  QTest::keyClick(window, Qt::Key_F, Qt::ControlModifier);
  QTest::keyClick(window, Qt::Key_P);
  ASSERT_GT(document->property("searchResultCount").toInt(), 0);
  QTest::keyClick(window, Qt::Key_F3);
  const int searchedRevision = nav->session()->revision();
  ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
  file.write(("Updated\n\n" + source).toUtf8());
  file.close();
  for (int i = 0; i < 500 && nav->session()->revision() == searchedRevision; ++i) {
    QTest::qWait(10);
  }
  settle();
  ASSERT_GT(nav->session()->revision(), searchedRevision);
  EXPECT_TRUE(document->property("searchActive").toBool());
  EXPECT_EQ(document->property("currentSearchResult").toInt(), 0);
  EXPECT_LT(viewport->property("contentY").toDouble(), newRange * 0.1);
  QTest::keyClick(window, Qt::Key_Escape);
  const double before = viewport->property("contentY").toDouble();
  pane->setProperty("findVisible", true);
  nav->activate("#missing");
  EXPECT_TRUE(pane->property("findVisible").toBool());
  EXPECT_DOUBLE_EQ(viewport->property("contentY").toDouble(), before);
  EXPECT_TRUE(nav->error().contains("Heading not found"));
  nav->activate("#finish");
  EXPECT_FALSE(pane->property("findVisible").toBool());
  QTest::qWait(50);
  EXPECT_GT(viewport->property("contentY").toDouble(), before);
  nav->activate("b.md#missing");
  settle();
  EXPECT_EQ(nav->session()->title(), "b.md");
  EXPECT_DOUBLE_EQ(viewport->property("contentY").toDouble(), 0);
  EXPECT_TRUE(nav->error().contains("Heading not found"));
}

TEST(ApplicationReaderFeatures, KeyboardLinkTraversalActivatesThroughController) {
  ApplicationHarness harness;
  ASSERT_NE(harness.root, nullptr) << harness.error.toStdString();
  auto* nav = harness.root->findChild<NavigationController*>();
  auto* document = harness.root->findChild<QQuickItem*>(QStringLiteral("markdownDocument"));
  auto* pane = harness.root->findChild<QQuickItem*>(QStringLiteral("readerPane"));
  auto* window = qobject_cast<QQuickWindow*>(harness.root.get());
  ASSERT_NE(nav, nullptr);
  ASSERT_NE(document, nullptr);
  ASSERT_NE(pane, nullptr);
  ASSERT_NE(window, nullptr);
  int launches = 0;
  nav->setLauncher([&launches](const QUrl& url) {
    ++launches;
    return url.host() == "example.org";
  });
  pane->setProperty("markdown", "[first](https://example.org/one)\n\n[second](https://example.org/two)\n");
  QTest::qWait(50);
  ASSERT_TRUE(document->isVisible());
  ASSERT_GT(document->width(), 0);
  ASSERT_GT(document->height(), 0);
  ASSERT_TRUE(document->property("keyboardNavigationEnabled").toBool());
  window->requestActivate();
  QTest::qWait(100);
  document->forceActiveFocus(Qt::TabFocusReason);
  ASSERT_TRUE(document->hasActiveFocus());
  ASSERT_EQ(window->activeFocusItem(), document);
  QTest::keyClick(window, Qt::Key_Tab);
  QTest::keyClick(window, Qt::Key_Return);
  EXPECT_EQ(launches, 1) << "focus: "
                         << (window->activeFocusItem() != nullptr ? window->activeFocusItem()->metaObject()->className()
                                                                  : "none");
  QTest::keyClick(window, Qt::Key_Tab);
  QTest::keyClick(window, Qt::Key_Return);
  EXPECT_EQ(launches, 2);
}
