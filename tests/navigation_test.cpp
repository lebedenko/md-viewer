#include "navigationcontroller.h"

#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <gtest/gtest.h>

namespace {
bool waitReady(NavigationController& controller) {
  for (int i = 0; i < 500 && controller.session()->state() == DocumentSession::State::Loading; ++i) {
    QTest::qWait(10);
  }
  return controller.session()->state() == DocumentSession::State::Ready;
}
void write(const QString& path, const QByteArray& text) {
  QFile file(path);
  ASSERT_TRUE(file.open(QIODevice::WriteOnly));
  ASSERT_EQ(file.write(text), text.size());
}
}  // namespace
TEST(Navigation, ResolvesLinksRestoresHistoryAndKeepsFailuresOut) {
  QTemporaryDir dir;
  write(dir.filePath("a.md"), "# A");
  write(dir.filePath("B.MARKDOWN"), "# B");
  NavigationController nav;
  QSignalSpy restore(&nav, &NavigationController::restoreRequested);
  nav.openPath(dir.filePath("a.md"));
  ASSERT_TRUE(waitReady(nav));
  nav.setPosition(0.6);
  nav.activate("B.MARKDOWN#b");
  ASSERT_TRUE(waitReady(nav));
  EXPECT_EQ(nav.session()->title(), "B.MARKDOWN");
  EXPECT_EQ(restore.last()[1].toString(), "b");
  EXPECT_TRUE(nav.canGoBack());
  nav.back();
  ASSERT_TRUE(waitReady(nav));
  EXPECT_EQ(nav.session()->title(), "a.md");
  EXPECT_DOUBLE_EQ(restore.last()[0].toDouble(), 0.6);
  EXPECT_TRUE(nav.canGoForward());
  nav.activate("missing.md");
  EXPECT_FALSE(waitReady(nav));
  EXPECT_EQ(nav.session()->title(), "a.md");
  EXPECT_TRUE(nav.canGoForward());
  nav.activate("#a");
  EXPECT_FALSE(nav.canGoForward());
}
TEST(Navigation, DesktopLaunchIsValidatedAndStubbed) {
  NavigationController nav;
  int calls = 0;
  nav.setLauncher([&calls](const QUrl&) {
    ++calls;
    return false;
  });
  nav.activate("https://example.org/docs");
  EXPECT_EQ(calls, 1);
  EXPECT_TRUE(nav.error().contains("Cannot launch"));
  nav.activate("javascript:alert(1)");
  EXPECT_EQ(calls, 1);
  EXPECT_TRUE(nav.error().contains("Unsupported"));
  nav.activate("https:");
  EXPECT_EQ(calls, 1);
}
TEST(Navigation, CompetingLoadsAndMissingCurrentFragment) {
  QTemporaryDir dir;
  write(dir.filePath("a.md"), "# A");
  write(dir.filePath("b.md"), "# B");
  NavigationController nav;
  nav.openPath(dir.filePath("a.md"));
  nav.openPath(dir.filePath("b.md"));
  ASSERT_TRUE(waitReady(nav));
  EXPECT_EQ(nav.session()->title(), "b.md");
  EXPECT_FALSE(nav.canGoBack());
  QObject::connect(&nav, &NavigationController::restoreRequested, &nav,
                   [&nav](double, const QString& fragment, bool, bool) {
                     if (!fragment.isEmpty()) {
                       nav.missingFragment(fragment);
                     }
                   });
  nav.activate("#missing");
  EXPECT_FALSE(nav.canGoBack());
  EXPECT_TRUE(nav.error().contains("Heading not found"));
}

TEST(Navigation, InvalidExplicitOpenSupersedesPendingLoad) {
  QTemporaryDir dir;
  write(dir.filePath("a.md"), "# A");
  NavigationController nav;
  nav.openPath(dir.filePath("a.md"));
  nav.openPath("~other-user/file.md");
  QTest::qWait(50);
  EXPECT_EQ(nav.session()->state(), DocumentSession::State::Failed);
  EXPECT_EQ(nav.session()->revision(), 0);
  EXPECT_FALSE(nav.canGoBack());
}
