#include "clipboard.h"
#include "documentsession.h"

#include <QClipboard>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFont>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
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
  qmlRegisterType<DocumentSession>("MdViewer", 1, 0, "DocumentSession");
  qmlRegisterSingletonType<Clipboard>("MdViewer", 1, 0, "Clipboard",
                                      [](QQmlEngine*, QJSEngine*) -> QObject* { return new Clipboard; });
  QQmlEngine engine;
  engine.addImportPath(QString::fromUtf8(MD_VIEWER_QMARKDOWN_IMPORT_PATH));
  QQmlComponent component(&engine, QUrl::fromLocalFile(QString::fromUtf8(MD_VIEWER_MAIN_QML)));
  ASSERT_TRUE(component.isReady()) << component.errorString().toStdString();
  const std::unique_ptr<QObject> root(component.create());
  ASSERT_NE(root, nullptr) << component.errorString().toStdString();
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
