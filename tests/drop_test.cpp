#include "appconfig.h"
#include "application_harness.h"
#include "clipboard.h"
#include "documentsession.h"

#include <QCoreApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QElapsedTimer>
#include <QFile>
#include <QMimeData>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QTest>

#include <gtest/gtest.h>
#include <memory>

// Qt 6.8 requires the QPoint drag-enter constructor; retain compatibility with that baseline.
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
TEST(ApplicationDrops, AcceptsCopyRequestsAndRejectsUnsupportedDrops) {
  ApplicationHarness harness;
  const auto& root = harness.root;
  ASSERT_NE(root, nullptr) << harness.error.toStdString();
  auto* window = qobject_cast<QQuickWindow*>(root.get());
  ASSERT_NE(window, nullptr);
  auto* session = root->findChild<DocumentSession*>();
  ASSERT_NE(session, nullptr);
  QCoreApplication::processEvents();
  QTemporaryDir dir;
  const QString path = dir.filePath("drop.md");
  QFile file(path);
  ASSERT_TRUE(file.open(QIODevice::WriteOnly));
  file.write("dropped content");
  file.close();

  const auto send = [window](const QMimeData& mime, Qt::DropActions actions, bool accepted) {
    QDragEnterEvent enter(QPoint(200, 200), actions, &mime, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &enter);
    EXPECT_EQ(enter.isAccepted(), !mime.urls().isEmpty() && actions.testFlag(Qt::CopyAction));
    QDropEvent drop(QPointF(200, 200), actions, &mime, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &drop);
    EXPECT_EQ(drop.isAccepted(), accepted);
    if (accepted) {
      EXPECT_EQ(drop.dropAction(), Qt::CopyAction);
    }
  };
  QMimeData local;
  local.setUrls({QUrl::fromLocalFile(path)});
  send(local, Qt::CopyAction, true);
  QElapsedTimer timer;
  timer.start();
  while (session->state() == DocumentSession::State::Loading && timer.elapsed() < 5000) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
  }
  EXPECT_EQ(session->markdown(), QStringLiteral("dropped content"));
  EXPECT_EQ(session->revision(), 1);
  // Shift proposes Move while Copy remains supported.
  QDragEnterEvent enter(QPoint(200, 200), Qt::CopyAction | Qt::MoveAction, &local, Qt::LeftButton, Qt::ShiftModifier);
  QCoreApplication::sendEvent(window, &enter);
  QDropEvent move(QPointF(200, 200), Qt::CopyAction | Qt::MoveAction, &local, Qt::LeftButton, Qt::ShiftModifier);
  ASSERT_EQ(move.proposedAction(), Qt::MoveAction);
  QCoreApplication::sendEvent(window, &move);
  EXPECT_TRUE(move.isAccepted());
  EXPECT_EQ(move.dropAction(), Qt::CopyAction);
  timer.restart();
  while (session->state() == DocumentSession::State::Loading && timer.elapsed() < 5000) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
  }
  EXPECT_EQ(session->revision(), 2);
  send(local, Qt::MoveAction, false);
  QMimeData text;
  text.setText(path);
  send(text, Qt::CopyAction, false);
  QMimeData empty;
  empty.setData("text/uri-list", QByteArray());
  send(empty, Qt::CopyAction, false);
  QMimeData remote;
  remote.setUrls({QUrl(QStringLiteral("https://example.org/doc.md"))});
  send(remote, Qt::CopyAction, false);
  EXPECT_EQ(session->state(), DocumentSession::State::Failed);
  EXPECT_TRUE(session->error().contains("Only local files"));
  EXPECT_EQ(session->markdown(), QStringLiteral("dropped content"));
  EXPECT_EQ(session->revision(), 2);
}

QT_WARNING_POP
