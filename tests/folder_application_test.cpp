#include "application_harness.h"
#include "documentsession.h"
#include "foldermodel.h"
#include "navigationcontroller.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTest>

#include <gtest/gtest.h>
#include <string_view>

namespace {
void writeDocument(const QString& path, const QByteArray& content) {
  ASSERT_TRUE(QDir().mkpath(QFileInfo(path).absolutePath()));
  QFile file(path);
  ASSERT_TRUE(file.open(QIODevice::WriteOnly));
  file.write(content);
}
// Delegates belong to the visual hierarchy only.
// NOLINTNEXTLINE(misc-no-recursion)
void collectRows(QQuickItem* parent, QList<QQuickItem*>& rows) {
  for (auto* item : parent->childItems()) {
    if (item->property("entryName").isValid()) {
      rows.append(item);
    }
    collectRows(item, rows);
  }
}
QQuickItem* treeRow(QQuickItem* sidebar, const QString& name) {
  QList<QQuickItem*> rows;
  collectRows(sidebar, rows);
  for (auto* row : rows) {
    if (row->property("entryName").toString() == name) {
      return row;
    }
  }
  return nullptr;
}
void typeText(QQuickWindow* window, std::string_view text) {
  for (const char letter : text) {
    QTest::keyClick(window, letter);
  }
}
struct FolderFixture {
  ApplicationHarness harness;
  QTemporaryDir dir;
  QString root = dir.path();
  FolderModel* folder = nullptr;
  NavigationController* nav = nullptr;
  QQuickWindow* window = nullptr;
  QQuickItem* sidebar = nullptr;

  void SetUp() {
    ASSERT_NE(harness.root, nullptr) << harness.error.toStdString();
    folder = harness.root->findChild<FolderModel*>();
    nav = harness.root->findChild<NavigationController*>();
    window = qobject_cast<QQuickWindow*>(harness.root.get());
    sidebar = harness.root->findChild<QQuickItem*>("folderSidebar");
    ASSERT_NE(folder, nullptr);
    ASSERT_NE(nav, nullptr);
    ASSERT_NE(window, nullptr);
    ASSERT_NE(sidebar, nullptr);
    writeDocument(root + "/README.md", "# Readme\n");
    writeDocument(root + "/docs/guide.md", "# Guide\n");
    writeDocument(root + "/docs/setup.md", "# Setup\n");
    writeDocument(root + "/src/main.cpp", "int main() {}\n");
  }
  void open(const QString& location) const {
    ASSERT_TRUE(QMetaObject::invokeMethod(harness.root.get(), "openLocation", Q_ARG(QVariant, location)));
  }
  [[nodiscard]] QString document() const { return nav->session()->url().toLocalFile(); }
};
}  // namespace

TEST(ApplicationFolder, DirectoryArgumentShowsTreeAndOpensReadme) {
  FolderFixture fixture;
  fixture.SetUp();
  if (::testing::Test::HasFatalFailure()) {
    return;
  }
  auto* files = fixture.harness.root->findChild<QQuickItem*>("filesButton");
  auto* reader = fixture.harness.root->findChild<QQuickItem*>("readerPane");
  ASSERT_NE(files, nullptr);
  ASSERT_NE(reader, nullptr);
  EXPECT_FALSE(fixture.sidebar->isVisible());
  EXPECT_FALSE(files->property("checkable").toBool());
  EXPECT_DOUBLE_EQ(reader->x(), 0);

  fixture.open(fixture.root);
  ASSERT_TRUE(waitFor([&] { return fixture.folder->root() == fixture.root; }, 5000));
  ASSERT_TRUE(waitFor([&] { return fixture.document() == fixture.root + "/README.md"; }, 5000));
  EXPECT_TRUE(fixture.sidebar->isVisible());
  EXPECT_DOUBLE_EQ(fixture.sidebar->width(), 260);
  EXPECT_DOUBLE_EQ(reader->x(), 260);
  EXPECT_TRUE(files->property("checked").toBool());
  EXPECT_EQ(treeRow(fixture.sidebar, "src"), nullptr);
  ASSERT_NE(treeRow(fixture.sidebar, "docs"), nullptr);
  auto* readme = treeRow(fixture.sidebar, "README.md");
  ASSERT_NE(readme, nullptr);
  EXPECT_TRUE(readme->property("isCurrent").toBool());

  fixture.window->setWidth(480);
  QTest::qWait(30);
  EXPECT_DOUBLE_EQ(fixture.sidebar->width(), 160);
  double previous_right = 0;
  for (const auto* name : {"backButton", "forwardButton", "openButton", "filesButton", "findButton", "editButton",
                           "outlineButton", "settingsButton", "closeButton"}) {
    auto* button = fixture.harness.root->findChild<QQuickItem*>(name);
    ASSERT_NE(button, nullptr);
    const QPointF origin = button->mapToScene(QPointF());
    EXPECT_GE(origin.x(), previous_right);
    EXPECT_GE(button->width(), 36);
    previous_right = origin.x() + button->width();
    EXPECT_LE(previous_right, 480);
  }
  EXPECT_TRUE(QFile::exists(":/assets/files.svg"));

  fixture.window->requestActivate();
  QTest::qWait(100);
  QTest::keyClick(fixture.window, Qt::Key_B, Qt::ControlModifier);
  EXPECT_FALSE(fixture.sidebar->isVisible());
  EXPECT_DOUBLE_EQ(reader->x(), 0);
  QTest::keyClick(fixture.window, Qt::Key_B, Qt::ControlModifier);
  EXPECT_TRUE(fixture.sidebar->isVisible());

  // A file argument is still a document, and a document under the folder is not replaced by the README.
  fixture.open(fixture.root + "/docs/guide.md");
  ASSERT_TRUE(waitFor([&] { return fixture.document() == fixture.root + "/docs/guide.md"; }, 5000));
  ASSERT_TRUE(waitFor([&] { return treeRow(fixture.sidebar, "guide.md") != nullptr; }, 2000));
  EXPECT_TRUE(treeRow(fixture.sidebar, "guide.md")->property("isCurrent").toBool());
  EXPECT_EQ(fixture.folder->root(), fixture.root);
}

TEST(ApplicationFolder, TreeKeyboardExpandsNavigatesAndOpens) {
  FolderFixture fixture;
  fixture.SetUp();
  if (::testing::Test::HasFatalFailure()) {
    return;
  }
  fixture.open(fixture.root);
  ASSERT_TRUE(waitFor([&] { return fixture.document() == fixture.root + "/README.md"; }, 5000));
  fixture.window->requestActivate();
  QTest::qWait(100);
  auto* docs = treeRow(fixture.sidebar, "docs");
  ASSERT_NE(docs, nullptr);
  docs->forceActiveFocus(Qt::TabFocusReason);
  ASSERT_TRUE(docs->hasActiveFocus());
  QTest::keyClick(fixture.window, Qt::Key_Right);
  ASSERT_TRUE(waitFor([&] { return treeRow(fixture.sidebar, "setup.md") != nullptr; }, 2000));
  EXPECT_TRUE(docs->property("expanded").toBool());
  QTest::keyClick(fixture.window, Qt::Key_Right);
  EXPECT_TRUE(treeRow(fixture.sidebar, "guide.md")->hasActiveFocus());
  QTest::keyClick(fixture.window, Qt::Key_Down);
  EXPECT_TRUE(treeRow(fixture.sidebar, "setup.md")->hasActiveFocus());
  QTest::keyClick(fixture.window, Qt::Key_Return);
  ASSERT_TRUE(waitFor([&] { return fixture.document() == fixture.root + "/docs/setup.md"; }, 5000));
  EXPECT_TRUE(fixture.nav->canGoBack());
  ASSERT_TRUE(waitFor([&] { return treeRow(fixture.sidebar, "setup.md")->property("isCurrent").toBool(); }, 2000));
  // Activation hands keyboard focus to the document, as outline activation does.
  EXPECT_FALSE(treeRow(fixture.sidebar, "setup.md")->hasActiveFocus());
  treeRow(fixture.sidebar, "setup.md")->forceActiveFocus(Qt::TabFocusReason);
  QTest::keyClick(fixture.window, Qt::Key_Left);
  EXPECT_TRUE(docs->hasActiveFocus());
  QTest::keyClick(fixture.window, Qt::Key_Left);
  ASSERT_TRUE(waitFor([&] { return treeRow(fixture.sidebar, "setup.md") == nullptr; }, 2000));
  QTest::keyClick(fixture.window, Qt::Key_Down);
  EXPECT_TRUE(treeRow(fixture.sidebar, "README.md")->hasActiveFocus());
  QTest::keyClick(fixture.window, Qt::Key_Space);
  ASSERT_TRUE(waitFor([&] { return fixture.document() == fixture.root + "/README.md"; }, 5000));
  fixture.nav->back();
  ASSERT_TRUE(waitFor([&] { return fixture.document() == fixture.root + "/docs/setup.md"; }, 5000));
}

TEST(ApplicationFolder, PickerFiltersOpensAndRestoresFocus) {
  FolderFixture fixture;
  fixture.SetUp();
  if (::testing::Test::HasFatalFailure()) {
    return;
  }
  auto* picker = fixture.harness.root->findChild<QObject*>("filePicker");
  ASSERT_NE(picker, nullptr);
  fixture.window->requestActivate();
  QTest::qWait(100);
  QTest::keyClick(fixture.window, Qt::Key_P, Qt::ControlModifier);
  EXPECT_FALSE(picker->property("visible").toBool());  // No folder yet.
  fixture.open(fixture.root);
  ASSERT_TRUE(waitFor([&] { return fixture.document() == fixture.root + "/README.md"; }, 5000));
  auto* readme = treeRow(fixture.sidebar, "README.md");
  ASSERT_NE(readme, nullptr);
  readme->forceActiveFocus(Qt::TabFocusReason);

  QTest::keyClick(fixture.window, Qt::Key_P, Qt::ControlModifier);
  ASSERT_TRUE(waitFor([&] { return picker->property("opened").toBool(); }, 2000));
  EXPECT_EQ(picker->property("results").toList().size(), 3);
  typeText(fixture.window, "zzz");
  ASSERT_TRUE(waitFor([&] { return picker->property("results").toList().isEmpty(); }, 2000));
  QTest::keyClick(fixture.window, Qt::Key_Return);
  EXPECT_TRUE(picker->property("visible").toBool());
  QTest::keyClick(fixture.window, Qt::Key_Escape);
  ASSERT_TRUE(waitFor([&] { return !picker->property("visible").toBool(); }, 2000));
  ASSERT_TRUE(waitFor([&] { return readme->hasActiveFocus(); }, 2000));
  EXPECT_EQ(fixture.document(), fixture.root + "/README.md");

  QTest::keyClick(fixture.window, Qt::Key_P, Qt::ControlModifier);
  ASSERT_TRUE(waitFor([&] { return picker->property("opened").toBool(); }, 2000));
  typeText(fixture.window, "docs");
  ASSERT_TRUE(waitFor([&] { return picker->property("results").toList().size() == 2; }, 2000));
  QTest::keyClick(fixture.window, Qt::Key_Down);
  QTest::keyClick(fixture.window, Qt::Key_Return);
  ASSERT_TRUE(waitFor([&] { return fixture.document() == fixture.root + "/docs/setup.md"; }, 5000));
  EXPECT_FALSE(picker->property("visible").toBool());
}

// Qt 6.8 requires the QPoint drag-enter constructor; retain compatibility with that baseline.
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
TEST(ApplicationFolder, DroppedFolderOpensAndFailuresKeepThePreviousOne) {
  FolderFixture fixture;
  fixture.SetUp();
  if (::testing::Test::HasFatalFailure()) {
    return;
  }
  QTemporaryDir plain;
  writeDocument(plain.path() + "/notes/only.md", "# Only\n");
  writeDocument(plain.path() + "/standalone.md", "# Standalone\n");
  fixture.open(plain.path() + "/standalone.md");
  ASSERT_TRUE(waitFor([&] { return fixture.document() == plain.path() + "/standalone.md"; }, 5000));
  EXPECT_TRUE(fixture.folder->root().isEmpty());
  EXPECT_FALSE(fixture.sidebar->isVisible());

  QMimeData mime;
  mime.setUrls({QUrl::fromLocalFile(fixture.root)});
  QDragEnterEvent enter(QPoint(200, 200), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
  QCoreApplication::sendEvent(fixture.window, &enter);
  QDropEvent drop(QPointF(200, 200), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
  QCoreApplication::sendEvent(fixture.window, &drop);
  EXPECT_TRUE(drop.isAccepted());
  ASSERT_TRUE(waitFor([&] { return fixture.folder->root() == fixture.root; }, 5000));
  ASSERT_TRUE(waitFor([&] { return fixture.document() == fixture.root + "/README.md"; }, 5000));

  // A folder without a root README keeps the current document.
  fixture.open(plain.path());
  ASSERT_TRUE(waitFor([&] { return fixture.folder->root() == plain.path(); }, 5000));
  QTest::qWait(100);
  EXPECT_EQ(fixture.document(), fixture.root + "/README.md");
  EXPECT_NE(treeRow(fixture.sidebar, "notes"), nullptr);

  ASSERT_TRUE(QDir(plain.path() + "/notes").removeRecursively());
  fixture.folder->openPath(plain.path() + "/notes");
  ASSERT_TRUE(waitFor(
      [&] { return fixture.harness.root->property("statusMessage").toString().contains("Folder not found"); }, 5000));
  EXPECT_EQ(fixture.folder->root(), plain.path());
  EXPECT_TRUE(fixture.sidebar->isVisible());
}
QT_WARNING_POP
