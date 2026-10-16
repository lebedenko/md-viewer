#include "appconfig.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

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
  EXPECT_EQ(AppConfig({fallback}).widthPreset(), 0);
}

TEST(AppConfig, PreservesUnknownKeysOnSave) {
  QTemporaryDir dir;
  const QString path = dir.filePath("config.toml");
  writeFile(path, "future = \"keep me\"\n[other]\nvalue = 7\n");
  AppConfig config({path});
  config.setZoom(1.1);
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
  EXPECT_TRUE(config.error().contains("not saved"));
}
