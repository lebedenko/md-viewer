#include "foldermodel.h"

#include "application_harness.h"

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <gtest/gtest.h>

namespace {
void writeFile(const QString& path, const QByteArray& content = "text\n") {
  ASSERT_TRUE(QDir().mkpath(QFileInfo(path).absolutePath()));
  QFile file(path);
  ASSERT_TRUE(file.open(QIODevice::WriteOnly));
  file.write(content);
}
QStringList rowNames(const FolderModel& model) {
  QStringList names;
  for (int row = 0; row < model.rowCount(); ++row) {
    const QModelIndex index = model.index(row);
    names.append(QString(model.data(index, static_cast<int>(FolderModel::Role::DepthRole)).toInt(), QLatin1Char(' ')) +
                 model.data(index, static_cast<int>(FolderModel::Role::NameRole)).toString());
  }
  return names;
}
int rowOf(const FolderModel& model, const QString& name) {
  for (int row = 0; row < model.rowCount(); ++row) {
    if (model.data(model.index(row), static_cast<int>(FolderModel::Role::NameRole)).toString() == name) {
      return row;
    }
  }
  return -1;
}
bool openAndWait(FolderModel& model, const QString& path) {
  QSignalSpy opened(&model, &FolderModel::opened);
  model.openPath(path);
  return opened.wait(5000);
}
}  // namespace

TEST(FolderScan, ListsMarkdownAndOnlyTheFoldersThatLeadToIt) {
  QTemporaryDir dir;
  const QString root = dir.path();
  for (const auto* file :
       {"README.md", "Zeta.md", "file10.md", "file2.md", "notes.txt", "docs/guide.MD", "docs/deep/a/b/page.markdown",
        "src/main.cpp", "src/lib/util.h", ".hidden/secret.md", ".dot.md", "assets/img/logo.svg"}) {
    writeFile(root + QLatin1Char('/') + QLatin1String(file));
  }
  ASSERT_TRUE(QDir(root).mkdir("empty"));
  const FolderModel::Scan scan = FolderModel::scan(root);
  EXPECT_TRUE(scan.error.isEmpty());
  EXPECT_FALSE(scan.truncated);
  EXPECT_EQ(scan.files, (QStringList{"docs/deep/a/b/page.markdown", "docs/guide.MD", "file2.md", "file10.md",
                                     "README.md", "Zeta.md"}));
  EXPECT_EQ(scan.directories,
            (QStringList{root, root + "/docs", root + "/docs/deep", root + "/docs/deep/a", root + "/docs/deep/a/b"}));
}

TEST(FolderScan, FollowsLinksStopsCyclesAndOmitsBrokenLinks) {
  QTemporaryDir dir;
  QTemporaryDir outside;
  const QString root = dir.path();
  writeFile(root + "/README.md");
  writeFile(root + "/docs/guide.md");
  writeFile(outside.path() + "/external.md");
  ASSERT_TRUE(QFile::link(root + "/docs", root + "/linked"));
  ASSERT_TRUE(QFile::link(outside.path(), root + "/outside"));
  ASSERT_TRUE(QFile::link(root, root + "/docs/loop"));
  ASSERT_TRUE(QFile::link(root + "/README.md", root + "/alias.md"));
  ASSERT_TRUE(QFile::link(root + "/missing.md", root + "/broken.md"));
  ASSERT_TRUE(QFile::link(root + "/missing", root + "/brokendir"));
  const FolderModel::Scan scan = FolderModel::scan(root);
  EXPECT_EQ(scan.files,
            (QStringList{"docs/guide.md", "linked/guide.md", "outside/external.md", "alias.md", "README.md"}));
}

TEST(FolderScan, ReportsInvalidRootsAndTruncation) {
  QTemporaryDir dir;
  const QString root = dir.path();
  writeFile(root + "/a.md");
  writeFile(root + "/b.md");
  writeFile(root + "/sub/c.md");
  EXPECT_TRUE(FolderModel::scan(root + "/missing").error.contains("not found"));
  EXPECT_TRUE(FolderModel::scan(root + "/a.md").error.contains("Not a folder"));
  const FolderModel::Scan capped = FolderModel::scan(root, nullptr, 2);
  EXPECT_TRUE(capped.truncated);
  EXPECT_EQ(capped.files.size(), 2);
  std::atomic_bool cancel = true;
  EXPECT_TRUE(FolderModel::scan(root, &cancel).files.isEmpty());
}

TEST(FolderModelRows, ExpandCollapseRevealAndParentRows) {
  QTemporaryDir dir;
  const QString root = dir.path();
  writeFile(root + "/readme.MD");
  writeFile(root + "/docs/guide.md");
  writeFile(root + "/docs/api/index.md");
  writeFile(root + "/build/noise.txt");
  FolderModel model;
  EXPECT_TRUE(FolderModel::isDirectory(root));
  EXPECT_TRUE(FolderModel::isDirectory(QUrl::fromLocalFile(root).toString()));
  EXPECT_FALSE(FolderModel::isDirectory(root + "/readme.MD"));
  model.openPath(root);
  EXPECT_EQ(model.loading(), root);
  EXPECT_TRUE(model.root().isEmpty());
  QSignalSpy opened(&model, &FolderModel::opened);
  ASSERT_TRUE(opened.wait(5000));
  EXPECT_TRUE(model.loading().isEmpty());
  EXPECT_EQ(model.root(), root);
  EXPECT_EQ(model.name(), QFileInfo(root).fileName());
  EXPECT_EQ(model.readme(), QUrl::fromLocalFile(root + "/readme.MD"));
  EXPECT_EQ(rowNames(model), (QStringList{"docs", "readme.MD"}));
  model.toggle(0);
  EXPECT_EQ(rowNames(model), (QStringList{"docs", " api", " guide.md", "readme.MD"}));
  EXPECT_TRUE(model.data(model.index(0), static_cast<int>(FolderModel::Role::ExpandedRole)).toBool());
  model.toggle(1);
  EXPECT_EQ(rowNames(model), (QStringList{"docs", " api", "  index.md", " guide.md", "readme.MD"}));
  EXPECT_EQ(model.parentRow(2), 1);
  EXPECT_EQ(model.parentRow(3), 0);
  EXPECT_EQ(model.parentRow(4), -1);
  model.toggle(0);
  EXPECT_EQ(rowNames(model), (QStringList{"docs", "readme.MD"}));
  model.toggle(0);  // The nested expansion is remembered.
  EXPECT_EQ(model.rowCount(), 5);
  model.toggle(1);
  model.toggle(0);
  model.toggle(3);  // Files and invalid rows ignore toggling.
  model.toggle(99);
  EXPECT_EQ(rowNames(model), (QStringList{"docs", "readme.MD"}));

  model.setCurrent(QUrl::fromLocalFile(root + "/docs/api/index.md"));
  EXPECT_EQ(rowNames(model), (QStringList{"docs", " api", "  index.md", " guide.md", "readme.MD"}));
  EXPECT_EQ(model.currentRow(), 2);
  model.toggle(0);
  EXPECT_EQ(model.currentRow(), -1);
  model.setCurrent(QUrl::fromLocalFile(dir.path() + "-elsewhere/other.md"));
  EXPECT_EQ(model.currentRow(), -1);
  EXPECT_FALSE(model.contains(model.current()));
  EXPECT_TRUE(model.contains(QUrl::fromLocalFile(root + "/docs/guide.md")));
}

TEST(FolderModelRows, RevealsThroughAnAlreadyExpandedDescendant) {
  QTemporaryDir dir;
  const QString root = dir.path();
  writeFile(root + "/docs/api/first.md");
  writeFile(root + "/docs/api/second.md");
  FolderModel model;
  ASSERT_TRUE(openAndWait(model, root));
  model.setCurrent(QUrl::fromLocalFile(root + "/docs/api/first.md"));
  ASSERT_GE(model.currentRow(), 0);
  model.toggle(0);  // Collapse docs while api retains its expansion state.
  ASSERT_EQ(model.currentRow(), -1);
  model.setCurrent(QUrl::fromLocalFile(root + "/docs/api/second.md"));
  EXPECT_EQ(rowNames(model), (QStringList{"docs", " api", "  first.md", "  second.md"}));
  EXPECT_EQ(model.currentRow(), rowOf(model, "second.md"));
  EXPECT_GE(model.currentRow(), 0);
}

TEST(FolderModelRows, SearchRanksFileNamesBeforeDirectories) {
  QTemporaryDir dir;
  const QString root = dir.path();
  for (const auto* file : {"guide/intro.md", "guide/setup.md", "api/Guide.md", "notes.md"}) {
    writeFile(root + QLatin1Char('/') + QLatin1String(file));
  }
  FolderModel model;
  ASSERT_TRUE(openAndWait(model, root));
  const auto names = [&model](const QString& query, int limit = 50) {
    QStringList found;
    for (const QVariant& entry : model.search(query, limit)) {
      const QVariantMap map = entry.toMap();
      found.append(map.value("directory").toString() + ":" + map.value("name").toString());
    }
    return found;
  };
  EXPECT_EQ(names("GUIDE"), (QStringList{"api:Guide.md", "guide:intro.md", "guide:setup.md"}));
  EXPECT_EQ(names(""), (QStringList{"api:Guide.md", "guide:intro.md", "guide:setup.md", ":notes.md"}));
  EXPECT_EQ(names("guide", 2), (QStringList{"api:Guide.md", "guide:intro.md"}));
  EXPECT_TRUE(names("absent").isEmpty());
  EXPECT_EQ(model.search("notes").constFirst().toMap().value("url").toUrl(), QUrl::fromLocalFile(root + "/notes.md"));
}

TEST(FolderModelRows, WatchedChangesRefreshAndKeepExpansion) {
  QTemporaryDir dir;
  const QString root = dir.path();
  writeFile(root + "/docs/guide.md");
  writeFile(root + "/index.md");
  FolderModel model;
  ASSERT_TRUE(openAndWait(model, root));
  model.toggle(0);
  ASSERT_EQ(rowNames(model), (QStringList{"docs", " guide.md", "index.md"}));
  writeFile(root + "/docs/added.md");
  ASSERT_TRUE(
      waitFor([&] { return rowNames(model) == (QStringList{"docs", " added.md", " guide.md", "index.md"}); }, 5000));
  // A folder that gains its first Markdown file becomes listed.
  writeFile(root + "/later/new.md");
  ASSERT_TRUE(waitFor([&] { return rowOf(model, "later") == 3; }, 5000));
  ASSERT_TRUE(QFile::remove(root + "/docs/guide.md"));
  ASSERT_TRUE(QFile::rename(root + "/docs/added.md", root + "/docs/renamed.md"));
  ASSERT_TRUE(
      waitFor([&] { return rowNames(model) == (QStringList{"docs", " renamed.md", "later", "index.md"}); }, 5000));
}

TEST(FolderModelRows, ReattachesWatchesAfterReplacementWithIdenticalNames) {
  QTemporaryDir dir;
  QTemporaryDir replacement;
  const QString root = dir.path();
  writeFile(root + "/docs/guide.md");
  writeFile(replacement.path() + "/docs/guide.md");
  FolderModel model;
  ASSERT_TRUE(openAndWait(model, root));
  model.toggle(0);
  QSignalSpy resets(&model, &QAbstractItemModel::modelReset);
  const int revision = model.revision();
  ASSERT_TRUE(QDir().rename(root + "/docs", replacement.path() + "/old-docs"));
  ASSERT_TRUE(QDir().rename(replacement.path() + "/docs", root + "/docs"));
  // An unchanged scan has no completion notification. Allow its debounce and background work to settle
  // before creating the file, so an outstanding replacement rescan cannot discover it accidentally.
  QTest::qWait(1500);
  EXPECT_TRUE(resets.isEmpty());
  EXPECT_EQ(model.revision(), revision);
  writeFile(root + "/docs/added.md");
  ASSERT_TRUE(waitFor([&] { return rowOf(model, "added.md") >= 0; }));
  EXPECT_EQ(rowNames(model), (QStringList{"docs", " added.md", " guide.md"}));
}

TEST(FolderModelRows, FailedOpenKeepsThePreviousFolder) {
  QTemporaryDir dir;
  const QString root = dir.path();
  writeFile(root + "/index.md");
  FolderModel model;
  ASSERT_TRUE(openAndWait(model, root));
  QSignalSpy errors(&model, &FolderModel::errorOccurred);
  QSignalSpy opened(&model, &FolderModel::opened);
  model.openPath(root + "/missing");
  ASSERT_TRUE(errors.wait(5000));
  EXPECT_TRUE(errors.constFirst().constFirst().toString().contains("Folder not found"));
  EXPECT_EQ(model.root(), root);
  EXPECT_TRUE(model.loading().isEmpty());
  EXPECT_EQ(rowNames(model), (QStringList{"index.md"}));
  model.openPath(QStringLiteral("https://example.org/docs"));
  EXPECT_EQ(errors.size(), 2);
  EXPECT_TRUE(opened.isEmpty());

  QTemporaryDir empty;
  ASSERT_TRUE(openAndWait(model, empty.path()));
  EXPECT_EQ(model.root(), empty.path());
  EXPECT_EQ(model.rowCount(), 0);
  EXPECT_EQ(model.fileCount(), 0);
  EXPECT_TRUE(model.readme().isEmpty());
}
