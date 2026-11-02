#include "appconfig.h"
#include "application_harness.h"
#include "clipboard.h"
#include "documentsession.h"
#include "editorlauncher.h"
#include "navigationcontroller.h"

#include <QClipboard>
#include <QColor>
#include <QCoreApplication>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QElapsedTimer>
#include <QFile>
#include <QFont>
#include <QGuiApplication>
#include <QMimeData>
#include <QProcess>
#include <QProcessEnvironment>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlProperty>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSet>
#include <QTemporaryDir>
#include <QTest>

#include <gtest/gtest.h>
#include <memory>

namespace {
// Repeater delegates belong to the application's visual hierarchy, not necessarily its QObject hierarchy.
QQuickItem* settingsItem(QQuickItem* parent, const QString& name) {
  if (parent == nullptr) {
    return nullptr;
  }
  if (parent->objectName() == name) {
    return parent;
  }
  for (auto* child : parent->childItems()) {
    if (auto* found = settingsItem(child, name)) {
      return found;
    }
  }
  return nullptr;
}
QQuickItem* settingsItem(QObject* dialog, const QString& name) {
  if (auto* found = settingsItem(dialog->property("contentItem").value<QQuickItem*>(), name)) {
    return found;
  }
  return settingsItem(dialog->property("header").value<QQuickItem*>(), name);
}
}  // namespace

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

  auto* width_box = settingsItem(dialog, QStringLiteral("widthBox"));
  ASSERT_NE(width_box, nullptr);
  ASSERT_TRUE(QMetaObject::invokeMethod(width_box, "activated", Q_ARG(int, 2)));
  EXPECT_DOUBLE_EQ(reader->property("maxColumnWidth").toDouble(), 1000.0);

  auto* zoom_slider = settingsItem(dialog, QStringLiteral("zoomSlider"));
  ASSERT_NE(zoom_slider, nullptr);
  ASSERT_TRUE(zoom_slider->setProperty("value", 1.5));
  ASSERT_TRUE(QMetaObject::invokeMethod(zoom_slider, "moved"));
  EXPECT_DOUBLE_EQ(reader->property("zoom").toDouble(), 1.5);

  auto* theme_box = settingsItem(dialog, QStringLiteral("themeBox"));
  ASSERT_NE(theme_box, nullptr);
  ASSERT_TRUE(QMetaObject::invokeMethod(theme_box, "activated", Q_ARG(int, 2)));
  EXPECT_TRUE(reader->property("dark").toBool());
  ASSERT_TRUE(QMetaObject::invokeMethod(theme_box, "activated", Q_ARG(int, 1)));
  EXPECT_FALSE(reader->property("dark").toBool());

  ASSERT_TRUE(dialog->setProperty("currentTab", 1));
  QCoreApplication::processEvents();
  auto* spacing_slider = settingsItem(dialog, QStringLiteral("spacingSlider"));
  ASSERT_NE(spacing_slider, nullptr);
  ASSERT_TRUE(spacing_slider->setProperty("value", 1.5));
  ASSERT_TRUE(QMetaObject::invokeMethod(spacing_slider, "moved"));
  EXPECT_DOUBLE_EQ(reader->property("blockSpacingScale").toDouble(), 1.5);

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
  auto* nav = harness.root->findChild<NavigationController*>();
  ASSERT_NE(nav, nullptr);
  nav->openPath(QString::fromUtf8(MD_VIEWER_TEST_DATA) + "/real-document.md");
  QTRY_VERIFY_WITH_TIMEOUT(nav->session()->state() == DocumentSession::State::Ready, 5000);
  QTest::qWait(30);
  EXPECT_TRUE(pane->property("outlineVisible").toBool());
  QTest::keyClick(window, Qt::Key_O, Qt::ControlModifier | Qt::ShiftModifier);
  EXPECT_FALSE(pane->property("outlineVisible").toBool());
  QTest::keyClick(window, Qt::Key_F, Qt::ControlModifier);
  EXPECT_TRUE(document->property("searchActive").toBool());
  QTest::keyClick(window, Qt::Key_M);
  EXPECT_GT(document->property("searchResultCount").toInt(), 0);
  QTest::keyClick(window, Qt::Key_Escape);
  EXPECT_FALSE(document->property("searchActive").toBool());
  QTest::keyClick(window, Qt::Key_F, Qt::ControlModifier);
  EXPECT_EQ(document->property("searchQuery").toString(), "m");
  QTest::keyClick(window, Qt::Key_Escape);
  EXPECT_TRUE(document->property("selectionEnabled").toBool());
  QTest::keyClick(window, Qt::Key_A, Qt::ControlModifier);
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

TEST(ApplicationEditor, ShortcutLaunchesConfiguredEditorOnlyForOpenedFiles) {
  ApplicationHarness harness;
  ASSERT_NE(harness.root, nullptr) << harness.error.toStdString();
  auto* window = qobject_cast<QQuickWindow*>(harness.root.get());
  auto* nav = harness.root->findChild<NavigationController*>();
  auto* launcher = harness.root->findChild<EditorLauncher*>();
  auto* button = harness.root->findChild<QQuickItem*>(QStringLiteral("editButton"));
  ASSERT_NE(window, nullptr);
  ASSERT_NE(nav, nullptr);
  ASSERT_NE(launcher, nullptr);
  ASSERT_NE(button, nullptr);
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const QString marker = dir.filePath("marker.txt");
  const QString script = dir.filePath("editor.sh");
  QFile editor(script);
  ASSERT_TRUE(editor.open(QIODevice::WriteOnly));
  editor.write(QStringLiteral("#!/bin/sh\nprintf '%s' \"$1\" > '%1'\n").arg(marker).toUtf8());
  editor.close();
  ASSERT_TRUE(editor.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));
  launcher->setCommand({script, "{file}"});
  EXPECT_FALSE(button->isEnabled());
  QTest::keyClick(window, Qt::Key_E, Qt::ControlModifier);
  QTest::qWait(200);
  EXPECT_FALSE(QFileInfo::exists(marker));
  QFile document(dir.filePath("a.md"));
  ASSERT_TRUE(document.open(QIODevice::WriteOnly));
  document.write("# A\n");
  document.close();
  nav->openPath(document.fileName());
  QTRY_VERIFY_WITH_TIMEOUT(button->isEnabled(), 5000);
  QTest::keyClick(window, Qt::Key_E, Qt::ControlModifier);
  QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(marker) && QFileInfo(marker).size() > 0, 5000);
  QFile recorded(marker);
  ASSERT_TRUE(recorded.open(QIODevice::ReadOnly));
  EXPECT_EQ(QFileInfo(QString::fromUtf8(recorded.readAll())).fileName(), QStringLiteral("a.md"));
}

TEST(ApplicationEditor, LatestFailureWinsAndOnlyItsOriginCanClearIt) {
  ApplicationHarness harness;
  ASSERT_NE(harness.root, nullptr) << harness.error.toStdString();
  auto* window = qobject_cast<QQuickWindow*>(harness.root.get());
  auto* nav = harness.root->findChild<NavigationController*>();
  auto* launcher = harness.root->findChild<EditorLauncher*>();
  auto* button = harness.root->findChild<QQuickItem*>(QStringLiteral("editButton"));
  ASSERT_NE(window, nullptr);
  ASSERT_NE(nav, nullptr);
  ASSERT_NE(launcher, nullptr);
  ASSERT_NE(button, nullptr);
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const QString path = dir.filePath("a.md");
  QFile file(path);
  ASSERT_TRUE(file.open(QIODevice::WriteOnly));
  file.write("# A\n");
  file.close();
  nav->openPath(path);
  QTRY_VERIFY_WITH_TIMEOUT(nav->session()->state() == DocumentSession::State::Ready, 5000);
  launcher->setCommand({dir.filePath("missing-editor")});
  const auto status = [&] { return window->property("statusMessage").toString(); };
  nav->missingFragment("missing");
  EXPECT_TRUE(status().contains(nav->error()));
  ASSERT_TRUE(QMetaObject::invokeMethod(button, "clicked"));
  EXPECT_TRUE(status().contains(launcher->error()));
  const QString editorError = launcher->error();
  nav->setPosition(0.5);
  nav->activate("#a");
  EXPECT_EQ(status(), QString::fromUtf8("⚠ ") + editorError);
  nav->missingFragment("missing");
  EXPECT_TRUE(status().contains(nav->error()));
  window->requestActivate();
  QTest::qWait(100);
  QTest::keyClick(window, Qt::Key_E, Qt::ControlModifier);
  EXPECT_EQ(status(), QString::fromUtf8("⚠ ") + editorError);
  nav->openPath(dir.filePath("missing.md"));
  QTRY_VERIFY_WITH_TIMEOUT(nav->session()->state() == DocumentSession::State::Failed, 5000);
  EXPECT_TRUE(status().contains(nav->session()->error()));
  launcher->setCommand({"/bin/true"});
  ASSERT_TRUE(launcher->openInEditor(nav->session()->url()));
  EXPECT_TRUE(status().contains(nav->session()->error()));
  nav->openPath(path);
  QTRY_VERIFY_WITH_TIMEOUT(nav->session()->state() == DocumentSession::State::Ready, 5000);
  EXPECT_TRUE(status().isEmpty());
  launcher->setCommand({dir.filePath("missing-editor")});
  ASSERT_FALSE(launcher->openInEditor(nav->session()->url()));
  ASSERT_TRUE(QFile::remove(path));
  QTRY_VERIFY_WITH_TIMEOUT(nav->session()->state() == DocumentSession::State::Failed, 5000);
  EXPECT_TRUE(status().contains(nav->session()->error()));
  ASSERT_FALSE(launcher->openInEditor(nav->session()->url()));
  EXPECT_EQ(status(), QString::fromUtf8("⚠ ") + editorError);
  nav->missingFragment("missing");
  EXPECT_TRUE(status().contains(nav->error()));
  nav->activate("#a");
  EXPECT_TRUE(status().isEmpty());
}

TEST(ApplicationVisualDesign, EmptyStartupActionsAndSuccessfulEmptyFile) {
  ApplicationHarness harness;
  ASSERT_NE(harness.root, nullptr) << harness.error.toStdString();
  auto* nav = harness.root->findChild<NavigationController*>();
  auto* window = qobject_cast<QQuickWindow*>(harness.root.get());
  auto* prompt = harness.root->findChild<QQuickItem*>("openPrompt");
  auto* document = harness.root->findChild<QQuickItem*>("markdownDocument");
  ASSERT_NE(nav, nullptr);
  ASSERT_NE(window, nullptr);
  ASSERT_NE(prompt, nullptr);
  ASSERT_NE(document, nullptr);
  EXPECT_TRUE(prompt->isVisible());
  EXPECT_TRUE(document->property("markdown").toString().isEmpty());
  for (const auto* name : {"findButton", "editButton", "outlineButton", "backButton", "forwardButton"}) {
    auto* button = harness.root->findChild<QQuickItem*>(name);
    ASSERT_NE(button, nullptr);
    EXPECT_FALSE(button->isEnabled());
  }
  window->setWidth(480);
  QTest::qWait(30);
  double previous_right = 0;
  for (const auto* name : {"backButton", "forwardButton", "openButton", "findButton", "editButton", "outlineButton",
                           "settingsButton", "closeButton"}) {
    auto* button = harness.root->findChild<QQuickItem*>(name);
    ASSERT_NE(button, nullptr);
    const QPointF origin = button->mapToScene(QPointF());
    EXPECT_GE(origin.x(), previous_right);
    EXPECT_GE(button->width(), 36);
    EXPECT_GE(button->height(), 36);
    previous_right = origin.x() + button->width();
    EXPECT_LE(previous_right, 480);
    EXPECT_FALSE(button->property("label").toString().isEmpty());
    EXPECT_TRUE(QFile::exists(":/assets/" + button->property("iconKey").toString() + ".svg"));
  }
  nav->openPath(harness.config_home.filePath("missing.md"));
  QTRY_VERIFY_WITH_TIMEOUT(nav->session()->state() == DocumentSession::State::Failed, 5000);
  EXPECT_TRUE(prompt->isVisible());
  QFile empty(harness.config_home.filePath("empty.md"));
  ASSERT_TRUE(empty.open(QIODevice::WriteOnly));
  empty.close();
  nav->openPath(empty.fileName());
  EXPECT_EQ(nav->session()->state(), DocumentSession::State::Loading);
  QTRY_VERIFY_WITH_TIMEOUT(nav->session()->state() == DocumentSession::State::Ready, 5000);
  EXPECT_FALSE(prompt->isVisible());
  EXPECT_TRUE(document->property("markdown").toString().isEmpty());
  EXPECT_TRUE(harness.root->findChild<QQuickItem*>("findButton")->isEnabled());
  nav->openPath(harness.config_home.filePath("missing.md"));
  QTRY_VERIFY_WITH_TIMEOUT(nav->session()->state() == DocumentSession::State::Failed, 5000);
  EXPECT_FALSE(prompt->isVisible());
  EXPECT_EQ(nav->session()->url().toLocalFile(), empty.fileName());
}

TEST(ApplicationVisualDesign, HeaderVisibilityAndCloseFocusTransfer) {
  ApplicationHarness harness;
  ASSERT_NE(harness.root, nullptr) << harness.error.toStdString();
  auto* window = qobject_cast<QQuickWindow*>(harness.root.get());
  auto* title = harness.root->findChild<QQuickItem*>("headerTitle");
  auto* close = harness.root->findChild<QQuickItem*>("closeButton");
  auto* settings = harness.root->findChild<QQuickItem*>("settingsButton");
  auto* dialog = harness.root->findChild<QObject*>("settingsDialog");
  auto* config = harness.root->findChild<AppConfig*>();
  ASSERT_NE(window, nullptr);
  ASSERT_NE(title, nullptr);
  ASSERT_NE(close, nullptr);
  ASSERT_NE(settings, nullptr);
  ASSERT_NE(config, nullptr);
  ASSERT_NE(dialog, nullptr);
  ASSERT_TRUE(QDir().mkpath(QFileInfo(config->path()).absolutePath()));
  window->requestActivate();
  QTest::qWait(100);
  const auto write = [&](bool show_title, bool show_close) {
    QFile file(config->path());
    EXPECT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write(QByteArray("[header]\nshow_title = ") + (show_title ? "true" : "false") +
               "\nshow_close_button = " + (show_close ? "true" : "false") + "\n");
  };
  for (const bool show_title : {false, true}) {
    for (const bool show_close : {false, true}) {
      write(show_title, show_close);
      QTRY_VERIFY_WITH_TIMEOUT(title->isVisible() == show_title && close->isVisible() == show_close, 3000);
    }
  }
  close->forceActiveFocus(Qt::TabFocusReason);
  ASSERT_TRUE(close->hasActiveFocus());
  write(true, false);
  QTRY_VERIFY_WITH_TIMEOUT(!close->isVisible(), 3000);
  EXPECT_TRUE(settings->hasActiveFocus());
  write(true, true);
  QTRY_VERIFY_WITH_TIMEOUT(close->isVisible(), 3000);
  QTest::keyClick(window, Qt::Key_Comma, Qt::ControlModifier);
  QTRY_VERIFY_WITH_TIMEOUT(dialog->property("visible").toBool(), 1000);
  auto* focused = window->activeFocusItem();
  write(false, false);
  QTRY_VERIFY_WITH_TIMEOUT(!close->isVisible(), 3000);
  EXPECT_EQ(window->activeFocusItem(), focused);
  EXPECT_TRUE(dialog->property("visible").toBool());
}

TEST(ApplicationVisualDesign, RightOutlineKeyboardMarkerAndToggleRetention) {
  ApplicationHarness harness;
  ASSERT_NE(harness.root, nullptr) << harness.error.toStdString();
  auto* nav = harness.root->findChild<NavigationController*>();
  auto* pane = harness.root->findChild<QQuickItem*>("readerPane");
  auto* sidebar = harness.root->findChild<QQuickItem*>("outlineSidebar");
  auto* viewport = harness.root->findChild<QQuickItem*>("readerViewport");
  auto* window = qobject_cast<QQuickWindow*>(harness.root.get());
  ASSERT_NE(nav, nullptr);
  ASSERT_NE(pane, nullptr);
  ASSERT_NE(sidebar, nullptr);
  ASSERT_NE(viewport, nullptr);
  ASSERT_NE(window, nullptr);
  QFile file(harness.config_home.filePath("outline.md"));
  const auto open = [&](const QByteArray& content) {
    EXPECT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write(content);
    file.close();
    nav->openPath(file.fileName());
  };
  open("# First\n\n## Second with a very long title that should elide within the sidebar\n\n### Third\n");
  QTRY_VERIFY_WITH_TIMEOUT(nav->session()->state() == DocumentSession::State::Ready, 5000);
  QTest::qWait(100);
  EXPECT_TRUE(sidebar->isVisible());
  EXPECT_DOUBLE_EQ(sidebar->x(), viewport->x() + viewport->width());
  EXPECT_DOUBLE_EQ(sidebar->width(), 240);
  window->setWidth(480);
  QTest::qWait(30);
  EXPECT_DOUBLE_EQ(sidebar->width(), 160);
  QList<QQuickItem*> rows;
  const auto collect = [&rows](auto&& self, QQuickItem* parent) -> void {
    for (auto* item : parent->childItems()) {
      if (item->property("modelData").isValid()) {
        rows.append(item);
      }
      self(self, item);
    }
  };
  collect(collect, sidebar);
  ASSERT_EQ(rows.size(), 3);
  EXPECT_LT(rows[0]->property("leftPadding").toDouble(), rows[1]->property("leftPadding").toDouble());
  window->requestActivate();
  QTest::qWait(100);
  rows[0]->forceActiveFocus(Qt::TabFocusReason);
  QTest::keyClick(window, Qt::Key_Down);
  EXPECT_TRUE(rows[1]->hasActiveFocus());
  QTest::keyClick(window, Qt::Key_Return);
  QTRY_COMPARE_WITH_TIMEOUT(pane->property("activatedHeading").toInt(), 1, 1000);
  rows[2]->forceActiveFocus();
  QTest::keyClick(window, Qt::Key_Space);
  QTRY_COMPARE_WITH_TIMEOUT(pane->property("activatedHeading").toInt(), 2, 1000);
  pane->setProperty("outlineVisible", false);
  open("No headings\n");
  QTRY_VERIFY_WITH_TIMEOUT(nav->session()->markdown() == "No headings\n", 5000);
  EXPECT_EQ(pane->property("activatedHeading").toInt(), -1);
  EXPECT_FALSE(sidebar->isVisible());
  open("# Back\n");
  QTRY_VERIFY_WITH_TIMEOUT(nav->session()->markdown() == "# Back\n", 5000);
  EXPECT_FALSE(sidebar->isVisible());
  QTest::keyClick(window, Qt::Key_O, Qt::ControlModifier | Qt::ShiftModifier);
  EXPECT_TRUE(sidebar->isVisible());
}

TEST(ApplicationSettings, TabsKeyboardCloseIconAndFocusReturn) {
  ApplicationHarness harness;
  ASSERT_NE(harness.root, nullptr) << harness.error.toStdString();
  auto* window = qobject_cast<QQuickWindow*>(harness.root.get());
  auto* dialog = harness.root->findChild<QObject*>("settingsDialog");
  ASSERT_NE(dialog, nullptr);
  QTest::keyClick(window, Qt::Key_Comma, Qt::ControlModifier);
  QTest::qWait(50);
  auto* first = settingsItem(dialog, "settingsTab0");
  ASSERT_NE(first, nullptr);
  first->forceActiveFocus(Qt::TabFocusReason);
  QTest::keyClick(window, Qt::Key_Down);
  EXPECT_EQ(dialog->property("currentTab").toInt(), 1);
  QTest::keyClick(window, Qt::Key_End, Qt::ControlModifier);
  EXPECT_TRUE(dialog->property("visible").toBool());
  for (int index = 0; index < 7; ++index) {
    auto* tab = settingsItem(dialog, QString("settingsTab%1").arg(index));
    ASSERT_NE(tab, nullptr);
    ASSERT_TRUE(QMetaObject::invokeMethod(tab, "clicked"));
    EXPECT_EQ(dialog->property("currentTab").toInt(), index);
  }
  auto* close = settingsItem(dialog, "settingsCloseButton");
  ASSERT_NE(close, nullptr);
  close->forceActiveFocus(Qt::TabFocusReason);
  QTest::keyClick(window, Qt::Key_Space);
  QTest::qWait(200);
  EXPECT_FALSE(dialog->property("visible").toBool());
  EXPECT_EQ(window->activeFocusItem()->objectName(), "settingsButton");
}

TEST(ApplicationSettings, PublicOptionsSharedOverridesAndResolvedFontZoom) {
  ApplicationHarness harness("[reader]\nselection_enabled = false\n");
  ASSERT_NE(harness.root, nullptr) << harness.error.toStdString();
  auto* config = harness.root->findChild<AppConfig*>();
  auto* document = harness.root->findChild<QObject*>("markdownDocument");
  auto* window = qobject_cast<QQuickWindow*>(harness.root.get());
  ASSERT_NE(config, nullptr);
  ASSERT_NE(document, nullptr);
  EXPECT_TRUE(document->property("selectionEnabled").toBool());
  auto* navigation = harness.root->findChild<NavigationController*>();
  ASSERT_NE(navigation, nullptr);
  navigation->openPath(QString::fromUtf8(MD_VIEWER_TEST_DATA) + "/real-document.md");
  QTRY_VERIFY_WITH_TIMEOUT(navigation->session()->state() == DocumentSession::State::Ready, 5000);
  QTest::qWait(30);
  QTest::keyClick(window, Qt::Key_A, Qt::ControlModifier);
  EXPECT_FALSE(document->property("selectedText").toString().isEmpty());
  EXPECT_TRUE(QMetaObject::invokeMethod(document, "requestCopy"));
  EXPECT_EQ(QGuiApplication::clipboard()->text(), document->property("selectedText").toString());
  auto* style = document->property("style").value<QObject*>();
  ASSERT_NE(style, nullptr);
  const auto original = style->property("bodyFont").value<QFont>();
  config->setReaderSetting("style.bodyFont", QFont("Serif", 18, QFont::Bold, true));
  config->setReaderSetting("style.bodyColor", "#80336699");
  config->setZoom(1.5);
  EXPECT_DOUBLE_EQ(style->property("bodyFont").value<QFont>().pointSizeF(), 27);
  for (int preset = 0; preset <= 5; ++preset) {
    config->setReaderSetting("options.stylePreset", preset);
    config->setTheme(preset % 2 == 0 ? AppConfig::Light : AppConfig::Dark);
    EXPECT_EQ(style->property("bodyColor").value<QColor>(), QColor("#80336699"));
    EXPECT_DOUBLE_EQ(style->property("bodyFont").value<QFont>().pointSizeF(), 27);
  }
  config->resetReaderSetting("style.bodyFont");
  config->resetReaderSetting("style.bodyColor");
  config->resetReaderSetting("options.stylePreset");
  config->setTheme(AppConfig::System);
  config->setZoom(1);
  EXPECT_EQ(style->property("bodyFont").value<QFont>(), original);
  for (const auto& name :
       {"extendedAutolinksEnabled", "strikethroughEnabled", "tablesEnabled", "tagFilterEnabled", "taskListsEnabled"}) {
    config->setReaderSetting(QString("options.") + name, false);
    EXPECT_TRUE(document->property("gfmEnabled").toBool());
    config->setReaderSetting("options.gfmEnabled", false);
    EXPECT_FALSE(document->property(name).toBool());
    config->setReaderSetting(QString("options.") + name, true);
    EXPECT_TRUE(document->property(name).toBool());
    config->setReaderSetting("options.gfmEnabled", true);
  }
  config->setReaderSetting("options.inlineMathEnabled", true);
  config->setReaderSetting("options.footnotesEnabled", true);
  config->setReaderSetting("options.taskIndicatorsReplaceBullets", false);
  EXPECT_TRUE(document->property("inlineMathEnabled").toBool());
  EXPECT_TRUE(document->property("footnotesEnabled").toBool());
  EXPECT_FALSE(document->property("taskIndicatorsReplaceBullets").toBool());
  config->setReaderSetting("options.codeBlockHeightRatio", 0.7);
  EXPECT_DOUBLE_EQ(document->property("codeBlockHeightRatio").toDouble(), 0.7);
  auto* code = style->property("codeBlock").value<QObject*>();
  auto* table = style->property("table").value<QObject*>();
  auto* math = style->property("math").value<QObject*>();
  ASSERT_NE(code, nullptr);
  ASSERT_NE(table, nullptr);
  ASSERT_NE(math, nullptr);
  config->setReaderSetting("style.codeBlock.lineNumberColor", "#ff663399");
  config->setReaderSetting("style.table.borderWidth", 3);
  config->setReaderSetting("style.math.scale", 1.3);
  EXPECT_EQ(code->property("lineNumberColor").value<QColor>(), QColor("#ff663399"));
  EXPECT_DOUBLE_EQ(table->property("borderWidth").toDouble(), 3);
  EXPECT_DOUBLE_EQ(math->property("scale").toDouble(), 1.3);
  auto* syntax = document->property("syntaxTheme").value<QObject*>();
  ASSERT_NE(syntax, nullptr);
  config->setReaderSetting("syntax.keywordColor", "#ff123456");
  EXPECT_EQ(syntax->property("keywordColor").value<QColor>(), QColor("#ff123456"));
  config->resetReaderSetting("syntax.keywordColor");
  ASSERT_TRUE(config->flush());
  QFile file(config->path());
  ASSERT_TRUE(file.open(QIODevice::ReadOnly));
  EXPECT_FALSE(file.readAll().contains("selection_enabled"));
}

TEST(ApplicationSettings, EveryCanonicalStyleFieldBindsOnceToThePublicRenderer) {
  ApplicationHarness harness;
  ASSERT_NE(harness.root, nullptr) << harness.error.toStdString();
  auto* config = harness.root->findChild<AppConfig*>();
  auto* document = harness.root->findChild<QObject*>("markdownDocument");
  ASSERT_NE(config, nullptr);
  ASSERT_NE(document, nullptr);
  QSet<QString> keys;
  for (const auto& entry : AppConfig::settingFields()) {
    const auto field = entry.toMap();
    const auto key = field.value("key").toString();
    ASSERT_FALSE(keys.contains(key));
    keys.insert(key);
    if (key.startsWith("options.")) {
      continue;
    }
    auto parts = key.split('.');
    const auto group = parts.takeFirst();
    QObject* target = document;
    if (group == "style") {
      target = document->property("style").value<QObject*>();
    } else if (group == "syntax") {
      target = document->property("syntaxTheme").value<QObject*>();
    }
    while (parts.size() > 1) {
      ASSERT_NE(target, nullptr) << key.toStdString();
      target = target->property(parts.takeFirst().toUtf8()).value<QObject*>();
    }
    ASSERT_NE(target, nullptr) << key.toStdString();
    const auto property = parts.first().toUtf8();
    const auto kind = field.value("kind").toString();
    if (kind == "font") {
      config->setReaderSetting(key, QFont("Serif", 19, QFont::Bold, true));
      const auto font = target->property(property).value<QFont>();
      EXPECT_EQ(font.family(), "Serif") << key.toStdString();
      EXPECT_DOUBLE_EQ(font.pointSizeF(), 19) << key.toStdString();
      EXPECT_TRUE(font.bold()) << key.toStdString();
      EXPECT_TRUE(font.italic()) << key.toStdString();
    } else if (kind == "color") {
      config->setReaderSetting(key, "#80442266");
      EXPECT_EQ(target->property(property).value<QColor>(), QColor("#80442266")) << key.toStdString();
    } else if (kind == "bool") {
      const bool value = !target->property(property).toBool();
      config->setReaderSetting(key, value);
      EXPECT_EQ(target->property(property).toBool(), value) << key.toStdString();
    } else {
      config->setReaderSetting(key, 2);
      EXPECT_DOUBLE_EQ(target->property(property).toDouble(), 2) << key.toStdString();
    }
    config->resetReaderSetting(key);
  }
  for (const auto& alias : {"style.codeBlockFont", "style.inlineCodeFont", "style.h1Font", "style.h6Color"}) {
    EXPECT_FALSE(keys.contains(alias));
  }
}

TEST(ApplicationSettings, GfmIndividualControlsAndDetailedResetApplyLive) {
  ApplicationHarness harness;
  ASSERT_NE(harness.root, nullptr) << harness.error.toStdString();
  auto* dialog = harness.root->findChild<QObject*>("settingsDialog");
  auto* config = harness.root->findChild<AppConfig*>();
  auto* document = harness.root->findChild<QObject*>("markdownDocument");
  ASSERT_NE(dialog, nullptr);
  ASSERT_NE(config, nullptr);
  ASSERT_NE(document, nullptr);
  ASSERT_TRUE(QMetaObject::invokeMethod(dialog, "open"));
  ASSERT_TRUE(dialog->setProperty("currentTab", 6));
  QTest::qWait(50);
  auto* tables = settingsItem(dialog, "setting.options.tablesEnabled");
  auto* gfm = settingsItem(dialog, "control.options.gfmEnabled");
  auto* inline_math = settingsItem(dialog, "control.options.inlineMathEnabled");
  ASSERT_NE(tables, nullptr);
  ASSERT_NE(gfm, nullptr);
  ASSERT_NE(inline_math, nullptr);
  EXPECT_FALSE(tables->isEnabled());
  ASSERT_TRUE(gfm->setProperty("checked", false));
  ASSERT_TRUE(QMetaObject::invokeMethod(gfm, "toggled"));
  EXPECT_FALSE(document->property("gfmEnabled").toBool());
  EXPECT_TRUE(tables->isEnabled());
  EXPECT_FALSE(config->readerSetting("options.tablesEnabled").toBool());
  ASSERT_TRUE(inline_math->setProperty("checked", true));
  ASSERT_TRUE(QMetaObject::invokeMethod(inline_math, "toggled"));
  EXPECT_TRUE(document->property("inlineMathEnabled").toBool());
  auto* scale = settingsItem(dialog, "control.style.math.scale");
  auto* reset = settingsItem(dialog, "reset.style.math.scale");
  ASSERT_NE(scale, nullptr);
  ASSERT_NE(reset, nullptr);
  ASSERT_TRUE(scale->setProperty("value", 130));
  ASSERT_TRUE(QMetaObject::invokeMethod(scale, "valueModified"));
  auto* style = document->property("style").value<QObject*>();
  ASSERT_NE(style, nullptr);
  auto* math = style->property("math").value<QObject*>();
  ASSERT_NE(math, nullptr);
  EXPECT_DOUBLE_EQ(math->property("scale").toDouble(), 1.3);
  EXPECT_TRUE(reset->isEnabled());
  ASSERT_TRUE(QMetaObject::invokeMethod(reset, "clicked"));
  EXPECT_FALSE(config->readerSettings().contains("style.math.scale"));
  EXPECT_DOUBLE_EQ(math->property("scale").toDouble(), 1);
}

TEST(ApplicationSettings, RowsAlignAcrossPagesWithoutEmptySpaceAndUseInsetSwatches) {
  ApplicationHarness harness;
  ASSERT_NE(harness.root, nullptr) << harness.error.toStdString();
  auto* window = qobject_cast<QQuickWindow*>(harness.root.get());
  auto* dialog = harness.root->findChild<QObject*>("settingsDialog");
  ASSERT_NE(window, nullptr);
  ASSERT_NE(dialog, nullptr);
  ASSERT_TRUE(QMetaObject::invokeMethod(dialog, "open"));
  QTest::qWait(50);
  auto* content = dialog->property("contentItem").value<QQuickItem*>();
  ASSERT_NE(content, nullptr);
  auto* background = dialog->property("background").value<QQuickItem*>();
  ASSERT_NE(background, nullptr);
  EXPECT_GT(background->property("radius").toDouble(), 0);
  auto* header = dialog->property("header").value<QQuickItem*>();
  auto* close = settingsItem(dialog, "settingsCloseButton");
  ASSERT_NE(header, nullptr);
  ASSERT_NE(close, nullptr);
  const QPointF close_origin = close->mapToItem(header, QPointF());
  EXPECT_GT(close_origin.y(), 0);
  EXPECT_NEAR(header->width() - close_origin.x() - close->width(), close_origin.y(), 1);
  EXPECT_TRUE(QQmlProperty(close, "Controls.ToolTip.text", qmlContext(close)).read().toString().isEmpty());
  EXPECT_FALSE(QQmlProperty(close, "Controls.ToolTip.visible", qmlContext(close)).read().toBool());
  // Outer horizontal padding follows the Controls style; only tab pages add wider insets.
  EXPECT_DOUBLE_EQ(dialog->property("leftPadding").toDouble(), dialog->property("padding").toDouble());
  EXPECT_DOUBLE_EQ(dialog->property("rightPadding").toDouble(), dialog->property("padding").toDouble());
  auto* reset_zoom = settingsItem(dialog, "reset.general.zoom");
  auto* config = harness.root->findChild<AppConfig*>();
  ASSERT_NE(reset_zoom, nullptr);
  ASSERT_NE(config, nullptr);
  QQmlProperty tooltip_visible(reset_zoom, "Controls.ToolTip.visible", qmlContext(reset_zoom));
  QQmlProperty tooltip_text(reset_zoom, "Controls.ToolTip.text", qmlContext(reset_zoom));
  ASSERT_TRUE(tooltip_visible.isValid());
  ASSERT_TRUE(tooltip_text.isValid());
  EXPECT_FALSE(reset_zoom->isEnabled());
  EXPECT_FALSE(reset_zoom->property("hoverEnabled").toBool());
  EXPECT_TRUE(tooltip_text.read().toString().isEmpty());
  config->setZoom(1.5);
  EXPECT_TRUE(reset_zoom->isEnabled());
  const QPoint hover = reset_zoom->mapToScene(QPointF(reset_zoom->width() / 2, reset_zoom->height() / 2)).toPoint();
  QTest::mouseMove(window, hover);
  QTRY_VERIFY_WITH_TIMEOUT(reset_zoom->property("hovered").toBool(), 1000);
  QTRY_VERIFY_WITH_TIMEOUT(tooltip_visible.read().toBool(), 1500);
  config->setZoom(1);
  EXPECT_FALSE(reset_zoom->property("hoverEnabled").toBool());
  auto* reset_background = reset_zoom->property("background").value<QQuickItem*>();
  ASSERT_NE(reset_background, nullptr);
  EXPECT_FALSE(reset_background->isVisible());
  EXPECT_FALSE(tooltip_visible.read().toBool());
  EXPECT_TRUE(tooltip_text.read().toString().isEmpty());
  const QStringList keys = {"general.zoom", "general.width", "general.theme", "options.stylePreset"};
  qreal previous_bottom = -1;
  qreal label_x = -1;
  qreal control_x = -1;
  qreal reset_x = -1;
  qreal first_top = -1;
  for (const auto& key : keys) {
    auto* row = settingsItem(dialog, "setting." + key);
    auto* label = settingsItem(dialog, "label." + key);
    auto* reset = settingsItem(dialog, "reset." + key);
    ASSERT_NE(row, nullptr);
    ASSERT_NE(label, nullptr);
    ASSERT_NE(reset, nullptr);
    auto* control = settingsItem(dialog, row->property("controlName").toString());
    ASSERT_NE(control, nullptr);
    const QPointF origin = row->mapToItem(content, QPointF());
    if (previous_bottom >= 0) {
      EXPECT_NEAR(origin.y() - previous_bottom, 12, 1);
      EXPECT_NEAR(label->mapToItem(content, QPointF()).x(), label_x, 1);
      EXPECT_NEAR(control->mapToItem(content, QPointF()).x(), control_x, 1);
      EXPECT_NEAR(reset->mapToItem(content, QPointF()).x(), reset_x, 1);
    } else {
      first_top = origin.y();
      label_x = label->mapToItem(content, QPointF()).x();
      control_x = control->mapToItem(content, QPointF()).x();
      reset_x = reset->mapToItem(content, QPointF()).x();
    }
    previous_bottom = origin.y() + row->height();
    EXPECT_LT(label_x, control_x);
    EXPECT_LT(control_x, reset_x);
    EXPECT_EQ(reset->property("iconKey").toString(), "reset");
    EXPECT_TRUE(reset->property("label").toString().startsWith("Reset "));
  }
  if (qEnvironmentVariableIsSet("MDV_SETTINGS_CAPTURE")) {
    EXPECT_TRUE(window->grabWindow().save("/tmp/mdv22-settings-general.png"));
  }
  const QStringList first_keys = {"style.headings.h1.font", "general.wrap", "general.highlighting",
                                  "style.table.headerForeground", "general.math"};
  for (int index = 2; index <= 6; ++index) {
    dialog->setProperty("currentTab", index);
    QTest::qWait(20);
    auto* row = settingsItem(dialog, "setting." + first_keys[index - 2]);
    ASSERT_NE(row, nullptr);
    EXPECT_NEAR(row->mapToItem(content, QPointF()).y(), first_top, 1) << index;
  }
  dialog->setProperty("currentTab", 2);
  QTest::qWait(20);
  auto* button = settingsItem(dialog, "control.style.headings.h1.color");
  auto* swatch = settingsItem(dialog, "swatch.style.headings.h1.color");
  ASSERT_NE(button, nullptr);
  ASSERT_NE(swatch, nullptr);
  EXPECT_GE(button->width() - swatch->mapToItem(button, QPointF()).x() - swatch->width(), 12);
  if (qEnvironmentVariableIsSet("MDV_SETTINGS_CAPTURE")) {
    EXPECT_TRUE(window->grabWindow().save("/tmp/mdv22-settings-headings.png"));
  }
  window->resize(480, 420);
  QTest::qWait(50);
  EXPECT_LE(dialog->property("width").toDouble(), 448);
  EXPECT_LE(dialog->property("height").toDouble(), 388);
  auto* row = settingsItem(dialog, "setting.style.headings.h1.color");
  ASSERT_NE(row, nullptr);
  EXPECT_LE(row->mapToItem(content, QPointF()).x() + row->width(), content->width());
  if (qEnvironmentVariableIsSet("MDV_SETTINGS_CAPTURE")) {
    EXPECT_TRUE(window->grabWindow().save("/tmp/mdv22-settings-small.png"));
  }
}
