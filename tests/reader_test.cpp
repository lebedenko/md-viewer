#include <QCoreApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
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
