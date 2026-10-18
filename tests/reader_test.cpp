#include "appconfig.h"
#include "application_harness.h"
#include "clipboard.h"
#include "documentsession.h"

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
#include <QQmlEngine>
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
  auto* button = item->findChild<QObject*>(QStringLiteral("codeCopyButton"));
  ASSERT_NE(button, nullptr);
  ASSERT_TRUE(QMetaObject::invokeMethod(button, "clicked"));
  EXPECT_EQ(engine.evaluate("copies").toInt(), 1);
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
