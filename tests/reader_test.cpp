#include "appconfig.h"
#include "application_harness.h"
#include "clipboard.h"
#include "documentsession.h"

#include <QAccessible>
#include <QClipboard>
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QGuiApplication>
#include <QImage>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlExpression>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTest>
#include <QUrl>
#include <QVariant>

#include <gtest/gtest.h>
#include <memory>

TEST(ReaderIntegration, CopiesRenderedTextAndClearsSelectionOnReplacement) {
  QQmlEngine engine;
  engine.addImportPath(QString::fromUtf8(MD_VIEWER_QMARKDOWN_IMPORT_PATH));
  QQmlComponent component(&engine, QUrl::fromLocalFile(QString::fromUtf8(MD_VIEWER_READER_QML)));
  ASSERT_TRUE(component.isReady()) << component.errorString().toStdString();
  const std::unique_ptr<QObject> pane(component.createWithInitialProperties(
      {{QStringLiteral("width"), 640},
       {QStringLiteral("markdown"), QStringLiteral("# Heading\n\nHello **reader**.")}}));
  ASSERT_NE(pane, nullptr) << component.errorString().toStdString();
  auto* document = pane->findChild<QQuickItem*>(QStringLiteral("markdownDocument"));
  ASSERT_NE(document, nullptr);
  QCoreApplication::processEvents();
  ASSERT_TRUE(QMetaObject::invokeMethod(document, "selectAll"));
  const auto selected = document->property("selectedText").toString();
  EXPECT_TRUE(selected.contains(QStringLiteral("Heading")));
  EXPECT_TRUE(selected.contains(QStringLiteral("Hello reader.")));
  EXPECT_FALSE(selected.contains(QStringLiteral("**")));
  ASSERT_TRUE(pane->setProperty("markdown", QStringLiteral("Replacement")));
  EXPECT_TRUE(document->property("selectedText").toString().isEmpty());
}

TEST(ReaderIntegration, RendersGfmExtensionsWithoutRawMarkers) {
  QQmlEngine engine;
  engine.addImportPath(QString::fromUtf8(MD_VIEWER_QMARKDOWN_IMPORT_PATH));
  QQmlComponent component(&engine, QUrl::fromLocalFile(QString::fromUtf8(MD_VIEWER_READER_QML)));
  ASSERT_TRUE(component.isReady()) << component.errorString().toStdString();
  const std::unique_ptr<QObject> pane(component.createWithInitialProperties(
      {{QStringLiteral("width"), 640},
       {QStringLiteral("markdown"),
        QStringLiteral("| Name | State |\n| --- | --- |\n| Cell | ready |\n\n- [x] done ~~old~~ new\n")}}));
  ASSERT_NE(pane, nullptr) << component.errorString().toStdString();
  auto* document = pane->findChild<QQuickItem*>(QStringLiteral("markdownDocument"));
  ASSERT_NE(document, nullptr);
  QCoreApplication::processEvents();
  ASSERT_TRUE(QMetaObject::invokeMethod(document, "selectAll"));
  const auto selected = document->property("selectedText").toString();
  EXPECT_TRUE(selected.contains(QStringLiteral("Cell")));
  EXPECT_TRUE(selected.contains(QStringLiteral("ready")));
  EXPECT_FALSE(selected.contains(QStringLiteral("|")));
  EXPECT_FALSE(selected.contains(QStringLiteral("~~")));
}

TEST(ReaderIntegration, CopiesSelectionOfRealDocumentToClipboard) {
  ApplicationHarness harness;
  const auto& root = harness.root;
  ASSERT_NE(root, nullptr) << harness.error.toStdString();
  auto* session = root->findChild<DocumentSession*>();
  ASSERT_NE(session, nullptr);
  session->openPath(QString::fromUtf8(MD_VIEWER_TEST_DATA) + QStringLiteral("/real-document.md"));
  QElapsedTimer timer;
  timer.start();
  while (session->state() != DocumentSession::State::Ready && timer.elapsed() < 5000) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
  }
  ASSERT_EQ(session->state(), DocumentSession::State::Ready) << session->error().toStdString();
  auto* document = root->findChild<QQuickItem*>(QStringLiteral("markdownDocument"));
  ASSERT_NE(document, nullptr);
  QCoreApplication::processEvents();
  QGuiApplication::clipboard()->clear();
  ASSERT_TRUE(QMetaObject::invokeMethod(document, "selectAll"));
  ASSERT_TRUE(QMetaObject::invokeMethod(document, "requestCopy"));
  const auto copied = QGuiApplication::clipboard()->text();

  for (const char* expected : {"Release notes 2.4", "bold", "inline_code()", "日本語のテキスト", "🚀",
                               "nested bullet two", "ninja -C build", "return x * 42;", "Parser", "passes all",
                               "Selection copies plain text", "without inserting hard line breaks"}) {
    EXPECT_TRUE(copied.contains(QString::fromUtf8(expected))) << expected;
  }
  for (const char* marker : {"**", "```", "~~", "](https", "| :--", "> A quoted"}) {
    EXPECT_FALSE(copied.contains(QString::fromUtf8(marker))) << marker;
  }
  EXPECT_FALSE(copied.contains(QStringLiteral("wrap across several visual lines when the viewport is narrow,\n")));
}

TEST(ReaderIntegration, ZoomScalesRenderedHeightAndRestoresAtOne) {
  QQmlEngine engine;
  engine.addImportPath(QString::fromUtf8(MD_VIEWER_QMARKDOWN_IMPORT_PATH));
  QQmlComponent component(&engine, QUrl::fromLocalFile(QString::fromUtf8(MD_VIEWER_READER_QML)));
  ASSERT_TRUE(component.isReady()) << component.errorString().toStdString();
  const std::unique_ptr<QObject> pane(component.createWithInitialProperties(
      {{QStringLiteral("width"), 640},
       {QStringLiteral("markdown"), QStringLiteral("# Title\n\nA paragraph of body text.\n\n```sh\nls -l\n```\n")}}));
  ASSERT_NE(pane, nullptr) << component.errorString().toStdString();
  auto* document = pane->findChild<QQuickItem*>(QStringLiteral("markdownDocument"));
  ASSERT_NE(document, nullptr);
  QQuickWindow window;
  window.resize(640, 480);
  qobject_cast<QQuickItem*>(pane.get())->setParentItem(window.contentItem());
  window.show();
  const auto settle = [&window] {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 200);
    window.grabWindow();  // forces the layout polish a visible window would get on its next frame
  };
  settle();
  const qreal normal = document->height();
  ASSERT_GT(normal, 0);
  ASSERT_TRUE(pane->setProperty("zoom", 2.0));
  settle();
  EXPECT_GT(document->height(), normal * 1.3);
  ASSERT_TRUE(pane->setProperty("zoom", 1.0));
  settle();
  EXPECT_NEAR(document->height(), normal, 1.0);
}

TEST(ReaderFeatures, PublicSearchHeadingsSelectionAndBackendContract) {
  ApplicationHarness harness;
  ASSERT_NE(harness.root, nullptr) << harness.error.toStdString();
  auto* pane = harness.root->findChild<QQuickItem*>(QStringLiteral("readerPane"));
  auto* document = harness.root->findChild<QQuickItem*>(QStringLiteral("markdownDocument"));
  ASSERT_NE(pane, nullptr);
  ASSERT_NE(document, nullptr);
  EXPECT_TRUE(document->property("mathAvailable").toBool());
  EXPECT_TRUE(document->property("syntaxHighlightingAvailable").toBool());
  EXPECT_TRUE(document->property("mathEnabled").toBool());
  EXPECT_TRUE(document->property("syntaxHighlightingEnabled").toBool());
  EXPECT_FALSE(document->property("codeBlockWrapEnabled").toBool());
  EXPECT_TRUE(document->property("codeBlockLineNumbersVisible").toBool());
  EXPECT_TRUE(document->property("codeBlockHeightLimitEnabled").toBool());
  pane->setProperty("markdown", "# First\n\nneedle NEEDLE\n\n## Last\n\n```cpp\nint answer = 42;\n```\n");
  QCoreApplication::processEvents();
  document->setProperty("searchQuery", "needle");
  document->setProperty("searchActive", true);
  EXPECT_EQ(document->property("searchResultCount").toInt(), 2);
  document->setProperty("searchCaseSensitive", true);
  EXPECT_EQ(document->property("searchResultCount").toInt(), 1);
  ASSERT_TRUE(QMetaObject::invokeMethod(document, "searchNext"));
  EXPECT_EQ(document->property("currentSearchResult").toInt(), 0);
  bool found = false;
  ASSERT_TRUE(QMetaObject::invokeMethod(document, "navigateToFragment", Q_RETURN_ARG(bool, found),
                                        Q_ARG(QString, QStringLiteral("#last"))));
  EXPECT_TRUE(found);
  ASSERT_TRUE(QMetaObject::invokeMethod(document, "navigateToHeading", Q_RETURN_ARG(bool, found), Q_ARG(int, 0)));
  EXPECT_TRUE(found);
  ASSERT_TRUE(QMetaObject::invokeMethod(document, "navigateToFragment", Q_RETURN_ARG(bool, found),
                                        Q_ARG(QString, QStringLiteral("#absent"))));
  EXPECT_FALSE(found);
  document->setProperty("searchActive", false);
  ASSERT_TRUE(QMetaObject::invokeMethod(document, "selectAll"));
  const QString payload = document->property("selectedText").toString();
  QGuiApplication::clipboard()->clear();
  ASSERT_TRUE(QMetaObject::invokeMethod(document, "requestCopy"));
  EXPECT_EQ(QGuiApplication::clipboard()->text(), payload);
  EXPECT_TRUE(payload.contains("int answer = 42;"));
}

TEST(ReaderFeatures, FormulaCopyAndFindPreserveSelectionEndpoints) {
  ApplicationHarness harness;
  ASSERT_NE(harness.root, nullptr) << harness.error.toStdString();
  auto* pane = harness.root->findChild<QQuickItem*>(QStringLiteral("readerPane"));
  auto* document = harness.root->findChild<QQuickItem*>(QStringLiteral("markdownDocument"));
  ASSERT_NE(pane, nullptr);
  ASSERT_NE(document, nullptr);
  const QString tex = QStringLiteral("\\frac{a}{b} + c^2");
  pane->setProperty("markdown", "Before formula.\n\n$$\n" + tex + "\n$$\n\nAfter formula.");
  QCoreApplication::processEvents();
  ASSERT_TRUE(document->property("mathAvailable").toBool());
  ASSERT_TRUE(QMetaObject::invokeMethod(document, "selectAll"));
  const QString payload = document->property("selectedText").toString();
  EXPECT_EQ(payload, "Before formula.\n\n" + tex + "\n\nAfter formula.");
  QGuiApplication::clipboard()->clear();
  ASSERT_TRUE(QMetaObject::invokeMethod(document, "requestCopy"));
  EXPECT_EQ(QGuiApplication::clipboard()->text(), payload);
  ASSERT_TRUE(QMetaObject::invokeMethod(pane, "openFind"));
  document->setProperty("searchQuery", tex);
  EXPECT_EQ(document->property("searchResultCount").toInt(), 1);
  const int start = document->property("selectionStart").toInt();
  const int end = document->property("selectionEnd").toInt();
  ASSERT_GT(end, start);
  ASSERT_TRUE(QMetaObject::invokeMethod(pane, "closeFind"));
  EXPECT_FALSE(document->property("searchActive").toBool());
  EXPECT_EQ(document->property("selectionStart").toInt(), start);
  EXPECT_EQ(document->property("selectionEnd").toInt(), end);
  EXPECT_EQ(document->property("selectedText").toString(), payload);
}

TEST(ReaderFeatures, LocalImagesAreScopedAndRemoteResourcesStayDenied) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  ASSERT_TRUE(QDir(dir.path()).mkdir("allowed"));
  ASSERT_TRUE(QDir(dir.path()).mkdir("other"));
  QImage image(180, 180, QImage::Format_RGB32);
  image.fill(Qt::red);
  ASSERT_TRUE(image.save(dir.filePath("allowed/picture.png")));
  ASSERT_TRUE(image.save(dir.filePath("outside.png")));
  ASSERT_TRUE(QFile::link(dir.filePath("outside.png"), dir.filePath("allowed/escape.png")));
  QQmlEngine engine;
  engine.addImportPath(QString::fromUtf8(MD_VIEWER_QMARKDOWN_IMPORT_PATH));
  QQmlComponent component(&engine, QUrl::fromLocalFile(QString::fromUtf8(MD_VIEWER_READER_QML)));
  QQuickWindow window;
  window.resize(640, 480);
  const auto rootUrl = QUrl::fromLocalFile(dir.filePath("allowed/"));
  std::unique_ptr<QObject> pane(
      component.createWithInitialProperties({{"width", 640},
                                             {"height", 480},
                                             {"baseUrl", QUrl::fromLocalFile(dir.filePath("allowed/doc.md"))},
                                             {"imageRoots", QVariant::fromValue(QList<QUrl>{rootUrl})}}));
  ASSERT_NE(pane, nullptr) << component.errorString().toStdString();
  auto* document = pane->findChild<QQuickItem*>(QStringLiteral("markdownDocument"));
  ASSERT_NE(document, nullptr);
  qobject_cast<QQuickItem*>(pane.get())->setParentItem(window.contentItem());
  window.show();
  pane->setProperty("markdown", "![image](picture.png)");
  for (int i = 0; i < 100 && document->height() < 150; ++i) {
    QTest::qWait(10);
  }
  EXPECT_GE(document->height(), 150);
  for (const char* source :
       {"![escape](../outside.png)", "![symlink](escape.png)", "![remote](https://example.org/picture.png)"}) {
    pane->setProperty("markdown", source);
    QTest::qWait(30);
    EXPECT_LT(document->height(), 100) << source;
  }
  pane->setProperty("baseUrl", QUrl::fromLocalFile(dir.filePath("other/doc.md")));
  pane->setProperty("imageRoots", QVariant::fromValue(QList<QUrl>{QUrl::fromLocalFile(dir.filePath("other/"))}));
  pane->setProperty("markdown", "![old](../allowed/picture.png)");
  QTest::qWait(30);
  EXPECT_LT(document->height(), 100);
}

TEST(ReaderFeatures, CodeHeaderTracksReaderThemeOverride) {
  QQmlEngine engine;
  engine.addImportPath(QString::fromUtf8(MD_VIEWER_QMARKDOWN_IMPORT_PATH));
  QQmlComponent component(&engine, QUrl::fromLocalFile(QString::fromUtf8(MD_VIEWER_READER_QML)));
  std::unique_ptr<QObject> pane(component.create());
  ASSERT_NE(pane, nullptr) << component.errorString().toStdString();
  auto* document = pane->findChild<QQuickItem*>(QStringLiteral("markdownDocument"));
  ASSERT_NE(document, nullptr);
  auto* actions = document->property("codeBlockActions").value<QQmlComponent*>();
  ASSERT_NE(actions, nullptr);
  std::unique_ptr<QObject> header(
      actions->createWithInitialProperties({{"languageName", "C++"},
                                            {"codeText", "int answer = 42;\n"},
                                            {"requestCopy", QVariant::fromValue(engine.evaluate("(function() {})"))}},
                                           actions->creationContext()));
  ASSERT_NE(header, nullptr) << actions->errorString().toStdString();
  auto* button = header->findChild<QQuickItem*>(QStringLiteral("codeCopyButton"));
  auto* label = header->findChild<QQuickItem*>(QStringLiteral("codeLanguageLabel"));
  ASSERT_NE(button, nullptr);
  ASSERT_NE(label, nullptr);
  // Exercise the public action component across reader theme changes.
  for (bool dark : {false, true, false}) {
    const QColor paper(dark ? "#1e1f22" : "#ffffff");
    const QColor text(dark ? "#e3e5e8" : "#1f2328");
    pane->setProperty("paperColor", paper);
    pane->setProperty("textColor", text);
    QCoreApplication::processEvents();
    EXPECT_EQ(label->property("color").value<QColor>(), text);
    QQmlExpression iconColor(qmlContext(button), button, QStringLiteral("icon.color"));
    EXPECT_EQ(iconColor.evaluate().value<QColor>(), text);
    QQmlExpression buttonColor(qmlContext(button), button, QStringLiteral("palette.button"));
    EXPECT_EQ(buttonColor.evaluate().value<QColor>(), paper);
  }
}

TEST(ReaderFeatures, ApplicationOwnedCodeComponentsHonorCallbacks) {
  QQmlEngine engine;
  const QString directory = QFileInfo(QString::fromUtf8(MD_VIEWER_READER_QML)).absolutePath();
  QQmlComponent header(&engine, QUrl::fromLocalFile(directory + "/CodeHeader.qml"));
  engine.evaluate("var copies = 0; var lastPosition = -1;");
  std::unique_ptr<QObject> item(header.createWithInitialProperties(
      {{"languageName", "C++"},
       {"codeText", "int answer = 42;\n"},
       {"requestCopy", QVariant::fromValue(engine.evaluate("(function() { ++copies; })"))}}));
  ASSERT_NE(item, nullptr) << header.errorString().toStdString();
  auto* button = item->findChild<QQuickItem*>(QStringLiteral("codeCopyButton"));
  ASSERT_NE(button, nullptr);
  ASSERT_TRUE(QMetaObject::invokeMethod(button, "clicked"));
  EXPECT_EQ(engine.evaluate("copies").toInt(), 1);
  auto* label = item->findChild<QQuickItem*>(QStringLiteral("codeLanguageLabel"));
  ASSERT_NE(label, nullptr);
  QQuickWindow window;
  window.resize(400, 100);
  auto* headerItem = qobject_cast<QQuickItem*>(item.get());
  headerItem->setParentItem(window.contentItem());
  headerItem->setWidth(400);
  headerItem->setHeight(headerItem->implicitHeight());
  window.show();
  QTest::qWait(20);
  EXPECT_EQ(label->x(), 0);
  EXPECT_NEAR(button->x() + button->width(), headerItem->width(), 0.01);
  EXPECT_NEAR(button->x() - label->width(), 8, 0.01);
  headerItem->setWidth(200);
  EXPECT_NEAR(button->x() + button->width(), 200, 0.01);
  EXPECT_TRUE(QFile::exists(QStringLiteral(":/assets/copy.svg")));
  QQmlExpression iconSource(qmlContext(button), button, QStringLiteral("icon.source.toString()"));
  EXPECT_EQ(iconSource.evaluate().toString(), "qrc:/assets/copy.svg");
  EXPECT_EQ(button->property("padding").toInt(), 4);
  auto* accessible = QAccessible::queryAccessibleInterface(button);
  ASSERT_NE(accessible, nullptr);
  EXPECT_EQ(accessible->text(QAccessible::Name), QStringLiteral("Copy code"));
  button->forceActiveFocus(Qt::TabFocusReason);
  EXPECT_TRUE(button->hasActiveFocus());
  QTest::keyClick(&window, Qt::Key_Space);
  EXPECT_EQ(engine.evaluate("copies").toInt(), 2);
  item->setProperty("languageName", "");
  EXPECT_FALSE(label->isVisible());
  EXPECT_NEAR(button->x() + button->width(), headerItem->width(), 0.01);
  QQmlComponent bar(&engine, QUrl::fromLocalFile(directory + "/CodeScrollBar.qml"));
  std::unique_ptr<QObject> scrollbar(bar.createWithInitialProperties(
      {{"orientation", Qt::Vertical},
       {"size", 0.25},
       {"position", 0.0},
       {"active", true},
       {"requestPosition", QVariant::fromValue(engine.evaluate("(function(value) { lastPosition = value; })"))}}));
  ASSERT_NE(scrollbar, nullptr) << bar.errorString().toStdString();
  scrollbar->setProperty("pressed", true);
  scrollbar->setProperty("position", 0.5);
  EXPECT_DOUBLE_EQ(engine.evaluate("lastPosition").toNumber(), 0.5);
}

TEST(ReaderFeatures, DecorativeTaskArtworkTracksStateWithoutInputSemantics) {
  QQmlEngine engine;
  engine.addImportPath(QString::fromUtf8(MD_VIEWER_QMARKDOWN_IMPORT_PATH));
  QQmlComponent styleComponent(&engine);
  styleComponent.setData("import QMarkdown 1.18; MarkdownStyle {}", QUrl());
  std::unique_ptr<QObject> style(styleComponent.create());
  ASSERT_NE(style, nullptr) << styleComponent.errorString().toStdString();
  const QString directory = QFileInfo(QString::fromUtf8(MD_VIEWER_READER_QML)).absolutePath();
  QQmlComponent component(&engine, QUrl::fromLocalFile(directory + "/TaskIndicator.qml"));
  std::unique_ptr<QObject> adapter(
      component.createWithInitialProperties({{"checked", false}, {"markdownStyle", QVariant::fromValue(style.get())}}));
  ASSERT_NE(adapter, nullptr) << component.errorString().toStdString();
  auto* item = qobject_cast<QQuickItem*>(adapter.get());
  ASSERT_NE(item, nullptr);
  EXPECT_GT(item->implicitWidth(), 0);
  EXPECT_GT(item->implicitHeight(), 0);
  EXPECT_FALSE(item->activeFocusOnTab());
  QQmlExpression ignored(qmlContext(item), item, QStringLiteral("Accessible.ignored"));
  EXPECT_TRUE(ignored.evaluate().toBool());
  auto* control = adapter->findChild<QQuickItem*>(QStringLiteral("taskArtworkControl"));
  ASSERT_NE(control, nullptr);
  EXPECT_FALSE(control->isVisible());
  EXPECT_TRUE(control->isEnabled());
  EXPECT_EQ(control->property("focusPolicy").toInt(), Qt::NoFocus);
  QQmlExpression controlIgnored(qmlContext(control), control, QStringLiteral("Accessible.ignored"));
  EXPECT_TRUE(controlIgnored.evaluate().toBool());
  EXPECT_FALSE(control->property("checked").toBool());
  adapter->setProperty("checked", true);
  EXPECT_TRUE(control->property("checked").toBool());
  QQuickWindow window;
  item->setParentItem(window.contentItem());
  item->setSize(QSizeF(item->implicitWidth(), item->implicitHeight()));
  window.show();
  QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, QPoint(2, 2));
  QTest::keyClick(&window, Qt::Key_Space);
  EXPECT_TRUE(adapter->property("checked").toBool());
  EXPECT_TRUE(control->property("checked").toBool());
  EXPECT_FALSE(control->hasActiveFocus());
  const auto font = QFont(QStringLiteral("monospace"), 22);
  style->setProperty("bodyFont", font);
  EXPECT_EQ(control->property("font").value<QFont>().pointSize(), 22);
}

TEST(ReaderFeatures, PublicPresentationAdaptersAndCodePadding) {
  ApplicationHarness harness;
  ASSERT_NE(harness.root, nullptr) << harness.error.toStdString();
  auto* document = harness.root->findChild<QQuickItem*>(QStringLiteral("markdownDocument"));
  ASSERT_NE(document, nullptr);
  EXPECT_TRUE(document->property("taskIndicatorsReplaceBullets").toBool());
  EXPECT_NE(document->property("taskIndicator").value<QQmlComponent*>(), nullptr);
  auto* style = document->property("style").value<QObject*>();
  ASSERT_NE(style, nullptr);
  auto* codeBlock = style->property("codeBlock").value<QObject*>();
  ASSERT_NE(codeBlock, nullptr);
  EXPECT_DOUBLE_EQ(codeBlock->property("verticalPadding").toDouble(), 6);
}
