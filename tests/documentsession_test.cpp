#include "documentsession.h"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QTemporaryDir>

#include <gtest/gtest.h>
#include <sys/stat.h>

namespace {

void waitUntilSettled(const DocumentSession& session) {
  QElapsedTimer timer;
  timer.start();
  while (session.state() == DocumentSession::State::Loading && timer.elapsed() < 5000) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
  }
  ASSERT_NE(session.state(), DocumentSession::State::Loading);
}

class DocumentSessionTest : public testing::Test {
 protected:
  QString write(const QString& name, const QByteArray& bytes) {
    const QString path = filePath(name);
    QFile file(path);
    EXPECT_TRUE(file.open(QIODevice::WriteOnly));
    file.write(bytes);
    return path;
  }

  [[nodiscard]] QString filePath(const QString& name) const { return dir_.filePath(name); }
  [[nodiscard]] QString dirPath() const { return dir_.path(); }

 private:
  QTemporaryDir dir_;
};

}  // namespace

TEST(DocumentSessionPath, ExpandsHomeAndRejectsOtherUsers) {
  EXPECT_EQ(*DocumentSession::resolvePath(QStringLiteral("~/notes/a.md")),
            QDir::cleanPath(QDir::homePath() + "/notes/a.md"));
  EXPECT_EQ(*DocumentSession::resolvePath(QStringLiteral("~")), QDir::cleanPath(QDir::homePath()));
  EXPECT_FALSE(DocumentSession::resolvePath(QStringLiteral("~root/a.md")).has_value());
}

TEST(DocumentSessionPath, MakesRelativeAbsoluteAndAcceptsFileUrls) {
  EXPECT_EQ(*DocumentSession::resolvePath(QStringLiteral("sub/../a.md")),
            QDir::current().absoluteFilePath("sub/../a.md"));
  EXPECT_EQ(*DocumentSession::resolvePath(QStringLiteral("file:///tmp/a%20b.md")), QStringLiteral("/tmp/a b.md"));
  EXPECT_FALSE(DocumentSession::resolvePath(QStringLiteral("https://example.org/a.md")).has_value());
  EXPECT_FALSE(DocumentSession::resolvePath(QString()).has_value());
}

TEST(DocumentSessionPath, AbbreviatesHomeOnlyAtDirectoryBoundary) {
  const QString home = QDir::cleanPath(QDir::homePath());
  EXPECT_EQ(DocumentSession::abbreviateHome(home + "/notes/a.md"), QStringLiteral("~/notes/a.md"));
  EXPECT_EQ(DocumentSession::abbreviateHome(home), QStringLiteral("~"));
  EXPECT_EQ(DocumentSession::abbreviateHome(home + "-other/a.md"), home + "-other/a.md");
  EXPECT_EQ(DocumentSession::abbreviateHome(QStringLiteral("/etc/a.md")), QStringLiteral("/etc/a.md"));
}

TEST(DocumentSessionDecode, StripsBomAndFlagsInvalidBytes) {
  const auto bom = DocumentSession::decode(QByteArray("\xEF\xBB\xBF# T"));
  ASSERT_TRUE(bom.has_value());
  EXPECT_EQ(bom->text, QStringLiteral("# T"));
  EXPECT_FALSE(bom->lossy);
  const auto lossy =
      DocumentSession::decode(QByteArray("a\xFF"
                                         "b"));
  ASSERT_TRUE(lossy.has_value());
  EXPECT_EQ(lossy->text, QStringLiteral("a�"
                                        "b"));
  EXPECT_TRUE(lossy->lossy);
}

TEST(DocumentSessionDecode, RejectsUtf16AndUtf32) {
  EXPECT_FALSE(DocumentSession::decode(QByteArray("\xFF\xFE#\x00", 4)).has_value());
  EXPECT_FALSE(DocumentSession::decode(QByteArray("\xFE\xFF\x00#", 4)).has_value());
  EXPECT_FALSE(DocumentSession::decode(QByteArray("\xFF\xFE\x00\x00#\x00\x00\x00", 8)).has_value());
  EXPECT_FALSE(DocumentSession::decode(QByteArray("\x00\x00\xFE\xFF", 4)).has_value());
}

TEST_F(DocumentSessionTest, OpensFileAndReportsMetadata) {
  const QString path = write("doc.md", "# Hello\n");
  DocumentSession session;
  session.openPath(path);
  EXPECT_EQ(session.state(), DocumentSession::State::Loading);
  waitUntilSettled(session);
  EXPECT_EQ(session.state(), DocumentSession::State::Ready);
  EXPECT_EQ(session.markdown(), QStringLiteral("# Hello\n"));
  EXPECT_EQ(session.title(), QStringLiteral("doc.md"));
  EXPECT_EQ(session.url(), QUrl::fromLocalFile(path));
  EXPECT_EQ(session.revision(), 1);
}

TEST_F(DocumentSessionTest, FailureKeepsLastGoodDocument) {
  DocumentSession session;
  session.openPath(write("good.md", "good"));
  waitUntilSettled(session);
  ASSERT_EQ(session.state(), DocumentSession::State::Ready);

  const QStringList bad = {filePath("missing.md"), dirPath(), write("wide.md", QByteArray("\xFF\xFE#\x00", 4))};
  for (const QString& path : bad) {
    session.openPath(path);
    waitUntilSettled(session);
    EXPECT_EQ(session.state(), DocumentSession::State::Failed) << path.toStdString();
    EXPECT_FALSE(session.error().isEmpty());
    EXPECT_EQ(session.markdown(), QStringLiteral("good"));
    EXPECT_EQ(session.title(), QStringLiteral("good.md"));
  }
  session.open(QUrl(QStringLiteral("https://example.org/a.md")));
  EXPECT_EQ(session.state(), DocumentSession::State::Failed);
  EXPECT_EQ(session.markdown(), QStringLiteral("good"));
}

TEST_F(DocumentSessionTest, RejectsOversizeFile) {
  DocumentSession session;
  session.openPath(write("big.md", QByteArray(DocumentSession::kMaxBytes + 1, 'a')));
  waitUntilSettled(session);
  EXPECT_EQ(session.state(), DocumentSession::State::Failed);
  EXPECT_EQ(session.revision(), 0);
}

TEST_F(DocumentSessionTest, WarnsAboutInvalidUtf8) {
  DocumentSession session;
  session.openPath(write("bad.md", QByteArray("a\xFF"
                                              "b")));
  waitUntilSettled(session);
  EXPECT_EQ(session.state(), DocumentSession::State::Ready);
  EXPECT_FALSE(session.warning().isEmpty());
}

TEST_F(DocumentSessionTest, LastRequestWins) {
  const QString first = write("first.md", QByteArray(4 * 1024 * 1024, 'a'));
  const QString second = write("second.md", "second");
  DocumentSession session;
  session.openPath(first);
  session.openPath(second);
  waitUntilSettled(session);
  // Let the superseded worker finish and deliver its result.
  QElapsedTimer timer;
  timer.start();
  while (timer.elapsed() < 300) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
  }
  EXPECT_EQ(session.markdown(), QStringLiteral("second"));
  EXPECT_EQ(session.revision(), 1);
}

TEST_F(DocumentSessionTest, RejectsNamedPipeAndKeepsLastGoodDocument) {
  DocumentSession session;
  const QString good = write("good.md", "good");
  session.openPath(good);
  waitUntilSettled(session);
  ASSERT_EQ(session.state(), DocumentSession::State::Ready);
  const QString pipe = filePath("pipe.md");
  ASSERT_EQ(::mkfifo(QFile::encodeName(pipe).constData(), 0600), 0);
  QElapsedTimer timer;
  timer.start();
  session.openPath(pipe);
  waitUntilSettled(session);
  EXPECT_LT(timer.elapsed(), 2000);
  EXPECT_EQ(session.state(), DocumentSession::State::Failed);
  EXPECT_TRUE(session.error().contains("Not a file"));
  EXPECT_EQ(session.markdown(), QStringLiteral("good"));
  EXPECT_EQ(session.url(), QUrl::fromLocalFile(good));
  EXPECT_EQ(session.revision(), 1);
}

TEST_F(DocumentSessionTest, PreservesFilesystemSymlinkParentResolution) {
  ASSERT_TRUE(QDir(dirPath()).mkpath("target/sub"));
  write("doc.md", "lexical competitor");
  write("target/doc.md", "filesystem document");
  ASSERT_TRUE(QFile::link(filePath("target/sub"), filePath("link")));
  const QString absolute = filePath("link/../doc.md");
  const QString relative = QDir::current().relativeFilePath(dirPath()) + "/link/../doc.md";
  DocumentSession session;
  for (int route = 0; route < 4; ++route) {
    if (route == 0) {
      session.openPath(relative);
    } else if (route == 1) {
      session.openPath(absolute);
    } else if (route == 2) {
      session.open(QUrl::fromLocalFile(absolute));
    } else {
      session.openPath(QUrl::fromLocalFile(absolute).toString());
    }
    waitUntilSettled(session);
    ASSERT_EQ(session.state(), DocumentSession::State::Ready);
    EXPECT_EQ(session.markdown(), QStringLiteral("filesystem document"));
    EXPECT_EQ(session.url().toLocalFile(), route == 0 ? QDir::current().absoluteFilePath(relative) : absolute);
  }
  session.openPath(filePath("link"));
  waitUntilSettled(session);
  EXPECT_EQ(session.state(), DocumentSession::State::Failed);
  ASSERT_TRUE(QFile::link(filePath("target/doc.md"), filePath("file-link.md")));
  session.openPath(filePath("file-link.md"));
  waitUntilSettled(session);
  EXPECT_EQ(session.state(), DocumentSession::State::Ready);
}

namespace {

void pump(int millis) {
  QElapsedTimer timer;
  timer.start();
  while (timer.elapsed() < millis) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
  }
}

bool waitForRevision(const DocumentSession& session, int revision) {
  QElapsedTimer timer;
  timer.start();
  while (session.revision() < revision && timer.elapsed() < 5000) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
  }
  return session.revision() >= revision;
}

}  // namespace

TEST_F(DocumentSessionTest, ReloadsAfterOrdinaryWrite) {
  const QString path = write("live.md", "one");
  DocumentSession session;
  session.openPath(path);
  waitUntilSettled(session);
  EXPECT_FALSE(session.reloaded());
  write("live.md", "two");
  ASSERT_TRUE(waitForRevision(session, 2));
  EXPECT_EQ(session.markdown(), QStringLiteral("two"));
  EXPECT_TRUE(session.reloaded());
}

TEST_F(DocumentSessionTest, ReloadsAfterAtomicReplaceRepeatedly) {
  const QString path = write("live.md", "one");
  DocumentSession session;
  session.openPath(path);
  waitUntilSettled(session);
  for (int i = 2; i <= 3; ++i) {
    write("tmp.md", QByteArray("v") + QByteArray::number(i));
    ASSERT_EQ(::rename(QFile::encodeName(filePath("tmp.md")).constData(), QFile::encodeName(path).constData()), 0);
    ASSERT_TRUE(waitForRevision(session, i)) << i;
    EXPECT_EQ(session.markdown(), QStringLiteral("v%1").arg(i));
  }
}

TEST_F(DocumentSessionTest, IdenticalContentDoesNotBumpRevision) {
  const QString path = write("live.md", "same");
  DocumentSession session;
  session.openPath(path);
  waitUntilSettled(session);
  write("live.md", "same");
  pump(600);
  EXPECT_EQ(session.revision(), 1);
  EXPECT_EQ(session.state(), DocumentSession::State::Ready);
}

TEST_F(DocumentSessionTest, DeletedFileKeepsContentAndRecovers) {
  const QString path = write("live.md", "first");
  DocumentSession session;
  session.openPath(path);
  waitUntilSettled(session);
  ASSERT_TRUE(QFile::remove(path));
  pump(600);
  EXPECT_EQ(session.state(), DocumentSession::State::Failed);
  EXPECT_EQ(session.markdown(), QStringLiteral("first"));
  write("live.md", "second");
  ASSERT_TRUE(waitForRevision(session, 2));
  EXPECT_EQ(session.state(), DocumentSession::State::Ready);
  EXPECT_TRUE(session.error().isEmpty());
  EXPECT_EQ(session.markdown(), QStringLiteral("second"));
}

TEST_F(DocumentSessionTest, OpeningAnotherFileStopsWatchingTheOldOne) {
  const QString first = write("a.md", "a");
  DocumentSession session;
  session.openPath(first);
  waitUntilSettled(session);
  session.openPath(write("b.md", "b"));
  waitUntilSettled(session);
  write("a.md", "a changed");
  pump(600);
  EXPECT_EQ(session.markdown(), QStringLiteral("b"));
  EXPECT_EQ(session.revision(), 2);
  EXPECT_FALSE(session.reloaded());
}
