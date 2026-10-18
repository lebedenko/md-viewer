#include "appconfig.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <gtest/gtest.h>

namespace {

void writeFile(const QString& path, const QByteArray& bytes) {
  ASSERT_TRUE(QDir().mkpath(QFileInfo(path).absolutePath()));
  QFile file(path);
  ASSERT_TRUE(file.open(QIODevice::WriteOnly));
  file.write(bytes);
}

QByteArray readFile(const QString& path) {
  QFile file(path);
  return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

}  // namespace

TEST(AppConfig, CandidatePathsPreferAbsoluteXdgThenHomeFallback) {
  EXPECT_EQ(AppConfig::candidatePaths("/xdg", "/home/u"),
            (QStringList{"/xdg/md-viewer/config.toml", "/home/u/.config/md-viewer.toml"}));
  EXPECT_EQ(AppConfig::candidatePaths("", "/home/u"), (QStringList{"/home/u/.config/md-viewer.toml"}));
  EXPECT_EQ(AppConfig::candidatePaths("relative", "/home/u"), (QStringList{"/home/u/.config/md-viewer.toml"}));
}

TEST(AppConfig, ParsesValuesClampsAndFallsBackPerKey) {
  const auto valid = AppConfig::parse("[reader]\nzoom = 1.5\ncolumn_width = \"wide\"\n");
  ASSERT_TRUE(valid.has_value());
  EXPECT_DOUBLE_EQ(valid->zoom, 1.5);
  EXPECT_EQ(valid->width_preset, 2);
  EXPECT_DOUBLE_EQ(AppConfig::parse("[reader]\nzoom = 9\n")->zoom, 3.0);
  EXPECT_DOUBLE_EQ(AppConfig::parse("[reader]\nzoom = 0.01\n")->zoom, 0.5);
  const auto wrong = AppConfig::parse("[reader]\nzoom = \"big\"\ncolumn_width = \"huge\"\n");
  ASSERT_TRUE(wrong.has_value());
  EXPECT_DOUBLE_EQ(wrong->zoom, 1.0);
  EXPECT_EQ(wrong->width_preset, 1);
  EXPECT_TRUE(AppConfig::parse("").has_value());
  const auto broken = AppConfig::parse("[reader\nzoom = 2");
  ASSERT_FALSE(broken.has_value());
  EXPECT_TRUE(broken.error().startsWith("line "));
}

TEST(AppConfig, WarnsAboutUnknownKeysAndInvalidValues) {
  const auto result =
      AppConfig::parse("extra = 1\n[reader]\ncolumn_widt = \"narrow\"\nzoom = \"x\"\ncolumn_width = \"huge\"\n");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->warnings.size(), 4);
  EXPECT_TRUE(result->warnings.join('\n').contains("reader.column_widt"));
  EXPECT_TRUE(AppConfig::parse("[reader]\nzoom = 1.0\ncolumn_width = \"wide\"\n")->warnings.isEmpty());
}

TEST(AppConfig, ParsesTypographyAndThemeWithPerKeyFallback) {
  const auto valid = AppConfig::parse(
      "[reader]\ntheme = \"dark\"\nbody_font = \" Serif \"\ncode_font = \"Mono\"\n"
      "block_spacing = 9.0\nlink_underline = false\n");
  ASSERT_TRUE(valid.has_value());
  EXPECT_TRUE(valid->warnings.isEmpty());
  EXPECT_EQ(valid->theme, AppConfig::Dark);
  EXPECT_EQ(valid->body_font, "Serif");
  EXPECT_EQ(valid->code_font, "Mono");
  EXPECT_DOUBLE_EQ(valid->block_spacing, AppConfig::kMaxBlockSpacing);
  EXPECT_FALSE(valid->link_underline);

  const auto bad =
      AppConfig::parse("[reader]\ntheme = \"sepia\"\nbody_font = 3\nblock_spacing = \"x\"\nlink_underline = \"yes\"\n");
  ASSERT_TRUE(bad.has_value());
  EXPECT_EQ(bad->warnings.size(), 4);
  EXPECT_EQ(bad->theme, AppConfig::System);
  EXPECT_TRUE(bad->body_font.isEmpty());
  EXPECT_DOUBLE_EQ(bad->block_spacing, 1.0);
  EXPECT_TRUE(bad->link_underline);
}

TEST(AppConfig, TypographyAndThemeRoundTrip) {
  QTemporaryDir dir;
  const QString path = dir.filePath("config.toml");
  {
    AppConfig config({path});
    config.setTheme(AppConfig::Light);
    config.setBodyFont(QStringLiteral("Serif"));
    config.setCodeFont(QStringLiteral("Mono"));
    config.setBlockSpacing(1.5);
    config.setLinkUnderline(false);
    ASSERT_TRUE(config.flush());
  }
  const AppConfig reloaded({path});
  EXPECT_EQ(reloaded.theme(), AppConfig::Light);
  EXPECT_EQ(reloaded.bodyFont(), "Serif");
  EXPECT_EQ(reloaded.codeFont(), "Mono");
  EXPECT_DOUBLE_EQ(reloaded.blockSpacing(), 1.5);
  EXPECT_FALSE(reloaded.linkUnderline());
}

TEST(AppConfig, UsesDefaultsWithoutFileAndCreatesPrimaryOnChange) {
  QTemporaryDir dir;
  const QString primary = dir.filePath("xdg/md-viewer/config.toml");
  AppConfig config({primary, dir.filePath("home/.config/md-viewer.toml")});
  EXPECT_EQ(config.path(), primary);
  EXPECT_DOUBLE_EQ(config.zoom(), 1.0);
  EXPECT_EQ(config.widthPreset(), 1);
  EXPECT_FALSE(QFileInfo::exists(primary));
  config.setZoom(1.25);
  config.setWidthPreset(2);
  EXPECT_TRUE(config.error().isEmpty());
  ASSERT_TRUE(config.flush());
  const AppConfig reloaded({primary});
  EXPECT_DOUBLE_EQ(reloaded.zoom(), 1.25);
  EXPECT_EQ(reloaded.widthPreset(), 2);
}

TEST(AppConfig, ReadsAndWritesFallbackFileWhenPrimaryIsMissing) {
  QTemporaryDir dir;
  const QString fallback = dir.filePath("home/.config/md-viewer.toml");
  writeFile(fallback, "[reader]\nzoom = 2.0\n");
  AppConfig config({dir.filePath("xdg/md-viewer/config.toml"), fallback});
  EXPECT_EQ(config.path(), fallback);
  EXPECT_DOUBLE_EQ(config.zoom(), 2.0);
  config.setWidthPreset(0);
  EXPECT_FALSE(QFileInfo::exists(dir.filePath("xdg/md-viewer/config.toml")));
  ASSERT_TRUE(config.flush());
  EXPECT_EQ(AppConfig({fallback}).widthPreset(), 0);
}

TEST(AppConfig, PreservesUnknownKeysOnSave) {
  QTemporaryDir dir;
  const QString path = dir.filePath("config.toml");
  writeFile(path, "future = \"keep me\"\n[other]\nvalue = 7\n");
  AppConfig config({path});
  config.setZoom(1.1);
  ASSERT_TRUE(config.flush());
  const QByteArray saved = readFile(path);
  EXPECT_TRUE(saved.contains("keep me")) << saved.toStdString();
  EXPECT_TRUE(saved.contains("value = 7")) << saved.toStdString();
  EXPECT_DOUBLE_EQ(AppConfig({path}).zoom(), 1.1);
}

TEST(AppConfig, NeverOverwritesFileWithSyntaxError) {
  QTemporaryDir dir;
  const QString path = dir.filePath("config.toml");
  const QByteArray broken = "[reader\nzoom = ";
  writeFile(path, broken);
  AppConfig config({path});
  EXPECT_FALSE(config.error().isEmpty());
  EXPECT_DOUBLE_EQ(config.zoom(), 1.0);
  config.setZoom(2.0);
  EXPECT_DOUBLE_EQ(config.zoom(), 2.0);  // still applies for this run
  EXPECT_EQ(readFile(path), broken);
  EXPECT_FALSE(config.flush());
  EXPECT_TRUE(config.error().contains("not saved"));
}

TEST(AppConfig, DebouncesChangesAndPersistsLatestValues) {
  QTemporaryDir dir;
  const QString path = dir.filePath("config.toml");
  AppConfig config({path});
  QSignalSpy changed(&config, &AppConfig::zoomChanged);
  config.setZoom(1.1);
  EXPECT_DOUBLE_EQ(config.zoom(), 1.1);
  EXPECT_EQ(changed.count(), 1);
  QTest::qWait(150);
  config.setZoom(1.5);
  QTest::qWait(150);
  EXPECT_FALSE(QFileInfo::exists(path));
  QTest::qWait(200);
  EXPECT_DOUBLE_EQ(AppConfig({path}).zoom(), 1.5);
}

TEST(AppConfig, DestructionAndShutdownFlushPendingChanges) {
  QTemporaryDir dir;
  const QString path = dir.filePath("config.toml");
  {
    AppConfig config({path});
    config.setZoom(1.5);
  }
  EXPECT_DOUBLE_EQ(AppConfig({path}).zoom(), 1.5);
  AppConfig config({path});
  config.setZoom(2.0);
  ASSERT_TRUE(QMetaObject::invokeMethod(QCoreApplication::instance(), "aboutToQuit"));
  EXPECT_DOUBLE_EQ(AppConfig({path}).zoom(), 2.0);
}

TEST(AppConfig, NamedWidthsRoundTrip) {
  QTemporaryDir dir;
  const QString path = dir.filePath("config.toml");
  for (const auto preset : {AppConfig::Narrow, AppConfig::Comfortable, AppConfig::Wide}) {
    AppConfig config({path});
    config.setWidthPreset(preset);
    config.setZoom(1.1);
    ASSERT_TRUE(config.flush());
    EXPECT_EQ(AppConfig({path}).widthPreset(), preset);
  }
}

TEST(AppConfig, ProtectsMalformedFileChangedAfterLoading) {
  QTemporaryDir dir;
  const QString path = dir.filePath("config.toml");
  writeFile(path, "[reader]\nzoom = 1.0\n");
  AppConfig config({path});
  writeFile(path, "[broken");
  config.setZoom(2.0);
  EXPECT_FALSE(config.flush());
  EXPECT_EQ(readFile(path), "[broken");
}

TEST(AppConfig, ReadFailureDoesNotReplaceDirectory) {
  QTemporaryDir dir;
  const QString path = dir.filePath("config.toml");
  AppConfig config({path});
  ASSERT_TRUE(QDir().mkdir(path));
  config.setZoom(2.0);
  EXPECT_FALSE(config.flush());
  EXPECT_FALSE(config.error().isEmpty());
  EXPECT_TRUE(QFileInfo(path).isDir());
}

TEST(AppConfig, UnreadableFileAfterLoadingIsPreserved) {
  QTemporaryDir dir;
  const QString path = dir.filePath("config.toml");
  const QByteArray original = "future = 7\n";
  writeFile(path, original);
  AppConfig config({path});
  ASSERT_TRUE(QFile::setPermissions(path, {}));
  QFile probe(path);
  if (probe.open(QIODevice::ReadOnly)) {
    QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    GTEST_SKIP() << "Runner bypasses filesystem permissions";
  }
  config.setZoom(2.0);
  EXPECT_FALSE(config.flush());
  EXPECT_FALSE(config.error().isEmpty());
  ASSERT_TRUE(QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner));
  EXPECT_EQ(readFile(path), original);
  EXPECT_TRUE(config.flush());
}

TEST(AppConfig, ReaderFeaturesValidateAndRoundTrip) {
  const auto defaults = AppConfig::parse({});
  ASSERT_TRUE(defaults);
  EXPECT_TRUE(defaults->selection_enabled);
  EXPECT_TRUE(defaults->math_enabled);
  EXPECT_TRUE(defaults->highlighting_enabled);
  EXPECT_FALSE(defaults->code_wrapping);
  EXPECT_TRUE(defaults->line_numbers);
  EXPECT_TRUE(defaults->height_limiting);
  const auto invalid = AppConfig::parse("[reader]\nmath_enabled = 1\ncode_wrapping = 'yes'\n");
  ASSERT_TRUE(invalid);
  EXPECT_EQ(invalid->warnings.size(), 2);
  EXPECT_TRUE(invalid->math_enabled);
  QTemporaryDir dir;
  const QString path = dir.filePath("config.toml");
  {
    AppConfig config({path});
    config.setSelectionEnabled(false);
    config.setMathEnabled(false);
    config.setHighlightingEnabled(false);
    config.setCodeWrapping(true);
    config.setLineNumbers(false);
    config.setHeightLimiting(false);
    ASSERT_TRUE(config.flush());
  }
  AppConfig config({path});
  EXPECT_FALSE(config.selectionEnabled());
  EXPECT_FALSE(config.mathEnabled());
  EXPECT_FALSE(config.highlightingEnabled());
  EXPECT_TRUE(config.codeWrapping());
  EXPECT_FALSE(config.lineNumbers());
  EXPECT_FALSE(config.heightLimiting());
}
