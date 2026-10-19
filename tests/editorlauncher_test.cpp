#include "editorlauncher.h"

#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <gtest/gtest.h>

namespace {

// A shell script that records one argument per line and its working directory.
QString writeRecorder(const QTemporaryDir& dir, const QString& output) {
  const QString script = dir.filePath(QStringLiteral("editor.sh"));
  QFile file(script);
  EXPECT_TRUE(file.open(QIODevice::WriteOnly));
  file.write(QStringLiteral("#!/bin/sh\npwd > '%1'\nfor a in \"$@\"; do printf '%s\\n' \"$a\" >> '%1'; done\n")
                 .arg(output)
                 .toUtf8());
  file.close();
  EXPECT_TRUE(file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));
  return script;
}

}  // namespace

TEST(EditorLauncher, SubstitutesFilePlaceholderWithinSingleElements) {
  const auto result = EditorLauncher::buildInvocation({"code", "--goto", "{file}:1", "--new"}, "/tmp/a b/c.md");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->program, "code");
  EXPECT_EQ(result->arguments, (QStringList{"--goto", "/tmp/a b/c.md:1", "--new"}));
}

TEST(EditorLauncher, AppendsFileWithoutPlaceholderAndKeepsMetacharactersLiteral) {
  const QString nasty = QStringLiteral("/tmp/x; $(touch pwned) \"q\" 'r'.md");
  const auto result = EditorLauncher::buildInvocation({"vim-qt", "-f"}, nasty);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->arguments, (QStringList{"-f", nasty}));
}

TEST(EditorLauncher, RejectsEmptyCommand) {
  EXPECT_FALSE(EditorLauncher::buildInvocation({}, "/tmp/a.md").has_value());
  EXPECT_FALSE(EditorLauncher::buildInvocation({""}, "/tmp/a.md").has_value());
}

TEST(EditorLauncher, LaunchesConfiguredCommandWithExactArgumentsInDocumentDirectory) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const QString output = dir.filePath(QStringLiteral("argv.txt"));
  const QString document = dir.filePath(QStringLiteral("my doc; $(x).md"));
  ASSERT_TRUE(QFile(document).open(QIODevice::WriteOnly));
  EditorLauncher launcher;
  launcher.setCommand({writeRecorder(dir, output), "--open", "{file}"});
  ASSERT_TRUE(launcher.openInEditor(QUrl::fromLocalFile(document)));
  EXPECT_TRUE(launcher.error().isEmpty());
  QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(output) && QFileInfo(output).size() > 0, 5000);
  QTest::qWait(100);
  QFile result(output);
  ASSERT_TRUE(result.open(QIODevice::ReadOnly));
  const QStringList lines = QString::fromUtf8(result.readAll()).split('\n', Qt::SkipEmptyParts);
  EXPECT_EQ(lines, (QStringList{QFileInfo(dir.path()).canonicalFilePath(), "--open", document}));
}

TEST(EditorLauncher, ReportsMissingProgramAndClearsErrorOnSuccess) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const QUrl document = QUrl::fromLocalFile(dir.filePath(QStringLiteral("a.md")));
  EditorLauncher launcher;
  QSignalSpy spy(&launcher, &EditorLauncher::errorChanged);
  QSignalSpy failures(&launcher, &EditorLauncher::errorOccurred);
  launcher.setCommand({dir.filePath(QStringLiteral("missing-editor")), "{file}"});
  EXPECT_FALSE(launcher.openInEditor(document));
  EXPECT_FALSE(launcher.error().isEmpty());
  EXPECT_FALSE(launcher.openInEditor(document));
  EXPECT_EQ(failures.count(), 2);
  EXPECT_EQ(spy.count(), 1);
  launcher.setCommand({writeRecorder(dir, dir.filePath(QStringLiteral("out.txt")))});
  EXPECT_TRUE(launcher.openInEditor(document));
  EXPECT_TRUE(launcher.error().isEmpty());
  EXPECT_EQ(spy.count(), 2);
}

TEST(EditorLauncher, RejectsNonLocalDocuments) {
  EditorLauncher launcher;
  launcher.setCommand({"true"});
  EXPECT_FALSE(launcher.openInEditor(QUrl(QStringLiteral("https://example.org/a.md"))));
  EXPECT_FALSE(launcher.error().isEmpty());
}
