#include "appconfig.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <toml++/toml.hpp>

namespace {

struct WidthName {
  std::string_view name;
  AppConfig::WidthPreset value;
};
constexpr std::array<WidthName, AppConfig::kWidthPresetCount> kWidthNames = {
    WidthName{.name = "narrow", .value = AppConfig::Narrow},
    {.name = "comfortable", .value = AppConfig::Comfortable},
    {.name = "wide", .value = AppConfig::Wide}};

struct ThemeName {
  std::string_view name;
  AppConfig::Theme value;
};
constexpr std::array<ThemeName, AppConfig::kThemeCount> kThemeNames = {
    ThemeName{.name = "system", .value = AppConfig::System},
    {.name = "light", .value = AppConfig::Light},
    {.name = "dark", .value = AppConfig::Dark}};

bool isAbsent(const QString& path) {
  struct stat info{};
  return ::lstat(QFile::encodeName(path).constData(), &info) != 0 && errno == ENOENT;
}

QString toQString(std::string_view text) { return QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size())); }

void applyNumber(const toml::node& node, const char* key, double min, double max, double& target,
                 QStringList& warnings) {
  const auto number = node.value<double>();
  if (number && std::isfinite(*number)) {
    target = std::clamp(*number, min, max);
  } else {
    warnings.append(QStringLiteral("reader.%1 must be a number").arg(QLatin1StringView(key)));
  }
}

void applyString(const toml::node& node, const char* key, QString& target, QStringList& warnings) {
  if (const auto text = node.value<std::string_view>()) {
    target = toQString(*text).trimmed();
  } else {
    warnings.append(QStringLiteral("reader.%1 must be a string").arg(QLatin1StringView(key)));
  }
}

void applyTheme(const toml::node& node, AppConfig::Values& values) {
  const auto text = node.value<std::string_view>();
  const auto* const found = text ? std::ranges::find(kThemeNames, *text, &ThemeName::name) : kThemeNames.end();
  if (found != kThemeNames.end()) {
    values.theme = found->value;
  } else {
    values.warnings.append(QStringLiteral("reader.theme must be \"system\", \"light\" or \"dark\""));
  }
}

void applyWidth(const toml::node& node, AppConfig::Values& values) {
  const auto text = node.value<std::string_view>();
  const auto* const found = text ? std::ranges::find(kWidthNames, *text, &WidthName::name) : kWidthNames.end();
  if (found != kWidthNames.end()) {
    values.width_preset = found->value;
  } else {
    values.warnings.append(QStringLiteral("reader.column_width must be \"narrow\", \"comfortable\" or \"wide\""));
  }
}

void applyReader(const toml::table& reader, AppConfig::Values& values) {
  for (const auto& [key, node] : reader) {
    const std::string_view name = key.str();
    if (name == "zoom") {
      applyNumber(node, "zoom", AppConfig::kMinZoom, AppConfig::kMaxZoom, values.zoom, values.warnings);
    } else if (name == "column_width") {
      applyWidth(node, values);
    } else if (name == "theme") {
      applyTheme(node, values);
    } else if (name == "body_font") {
      applyString(node, "body_font", values.body_font, values.warnings);
    } else if (name == "code_font") {
      applyString(node, "code_font", values.code_font, values.warnings);
    } else if (name == "block_spacing") {
      applyNumber(node, "block_spacing", AppConfig::kMinBlockSpacing, AppConfig::kMaxBlockSpacing, values.block_spacing,
                  values.warnings);
    } else if (name == "link_underline") {
      if (const auto underline = node.value<bool>()) {
        values.link_underline = *underline;
      } else {
        values.warnings.append(QStringLiteral("reader.link_underline must be true or false"));
      }
    } else {
      values.warnings.append(QStringLiteral("unknown key 'reader.%1'").arg(toQString(name)));
    }
  }
}

}  // namespace

QStringList AppConfig::candidatePaths(const QString& xdg_config_home, const QString& home) {
  QStringList paths;
  if (QDir::isAbsolutePath(xdg_config_home)) {
    paths.append(QDir(xdg_config_home).filePath(QStringLiteral("md-viewer/config.toml")));
  }
  paths.append(QDir(home).filePath(QStringLiteral(".config/md-viewer.toml")));
  return paths;
}

std::expected<AppConfig::Values, QString> AppConfig::parse(const QByteArray& bytes) {
  toml::table table;
  try {
    table = toml::parse(std::string_view(bytes.constData(), static_cast<std::size_t>(bytes.size())));
  } catch (const toml::parse_error& e) {
    return std::unexpected(
        QStringLiteral("line %1: %2")
            .arg(e.source().begin.line)
            .arg(QString::fromUtf8(e.description().data(), static_cast<qsizetype>(e.description().size()))));
  }
  Values values;
  for (const auto& [key, node] : table) {
    if (key.str() != "reader") {
      values.warnings.append(
          QStringLiteral("unknown key '%1'").arg(QString::fromUtf8(key.data(), static_cast<qsizetype>(key.length()))));
    } else if (!node.is_table()) {
      values.warnings.append(QStringLiteral("'reader' must be a table"));
    }
  }
  if (const auto* reader = table["reader"].as_table()) {
    applyReader(*reader, values);
  }
  return values;
}

AppConfig::AppConfig(QObject* parent)
    : AppConfig(candidatePaths(qEnvironmentVariable("XDG_CONFIG_HOME"), QDir::homePath()), parent) {}

AppConfig::AppConfig(const QStringList& candidates, QObject* parent) : QObject(parent) {
  save_timer_.setSingleShot(true);
  save_timer_.setInterval(250);
  connect(&save_timer_, &QTimer::timeout, this, [this] { flush(); });
  if (auto* app = QCoreApplication::instance()) {
    connect(app, &QCoreApplication::aboutToQuit, this, [this] { flush(); });
  }
  load(candidates);
}

AppConfig::~AppConfig() { flush(); }

void AppConfig::scheduleSave() {
  pending_ = true;
  save_timer_.start();
}

bool AppConfig::flush() {
  save_timer_.stop();
  if (!pending_) {
    return true;
  }
  if (!save()) {
    return false;
  }
  pending_ = false;
  return true;
}

void AppConfig::load(const QStringList& candidates) {
  const auto existing = std::ranges::find_if(candidates, [](const QString& candidate) { return !isAbsent(candidate); });
  path_ = existing != candidates.end() ? *existing : candidates.value(0);
  if (existing == candidates.end()) {
    return;
  }
  QFile file(path_);
  if (!file.open(QIODevice::ReadOnly)) {
    readable_ = false;
    error_ = QStringLiteral("Cannot read %1: %2").arg(path_, file.errorString());
    qWarning().noquote() << "Configuration" << error_;
    return;
  }
  const QByteArray bytes = file.readAll();
  if (file.error() != QFileDevice::NoError) {
    readable_ = false;
    error_ = QStringLiteral("Cannot read %1: %2").arg(path_, file.errorString());
    return;
  }
  const auto parsed = parse(bytes);
  if (!parsed) {
    readable_ = false;
    error_ = QStringLiteral("%1: %2").arg(path_, parsed.error());
    qWarning().noquote() << "Configuration" << error_ << "- using defaults";
    return;
  }
  values_ = *parsed;
  for (const QString& warning : parsed->warnings) {
    qWarning().noquote() << "Configuration" << path_ << "-" << warning;
  }
}

void AppConfig::setZoom(double zoom) {
  zoom = std::clamp(zoom, kMinZoom, kMaxZoom);
  if (!std::isfinite(zoom) || zoom == values_.zoom) {
    return;
  }
  values_.zoom = zoom;
  emit zoomChanged();
  scheduleSave();
}

void AppConfig::setWidthPreset(int preset) {
  preset = std::clamp(preset, 0, kWidthPresetCount - 1);
  if (preset == values_.width_preset) {
    return;
  }
  values_.width_preset = preset;
  emit widthPresetChanged();
  scheduleSave();
}

void AppConfig::setTheme(int theme) {
  theme = std::clamp(theme, 0, kThemeCount - 1);
  if (theme == values_.theme) {
    return;
  }
  values_.theme = theme;
  emit themeChanged();
  scheduleSave();
}

void AppConfig::setBodyFont(const QString& family) {
  const QString trimmed = family.trimmed();
  if (trimmed == values_.body_font) {
    return;
  }
  values_.body_font = trimmed;
  emit bodyFontChanged();
  scheduleSave();
}

void AppConfig::setCodeFont(const QString& family) {
  const QString trimmed = family.trimmed();
  if (trimmed == values_.code_font) {
    return;
  }
  values_.code_font = trimmed;
  emit codeFontChanged();
  scheduleSave();
}

void AppConfig::setBlockSpacing(double spacing) {
  spacing = std::clamp(spacing, kMinBlockSpacing, kMaxBlockSpacing);
  if (!std::isfinite(spacing) || spacing == values_.block_spacing) {
    return;
  }
  values_.block_spacing = spacing;
  emit blockSpacingChanged();
  scheduleSave();
}

void AppConfig::setLinkUnderline(bool underline) {
  if (underline == values_.link_underline) {
    return;
  }
  values_.link_underline = underline;
  emit linkUnderlineChanged();
  scheduleSave();
}

void AppConfig::setError(const QString& error) {
  if (error != error_) {
    error_ = error;
    emit errorChanged();
  }
}

bool AppConfig::save() {
  if (path_.isEmpty()) {
    return false;
  }
  if (!readable_) {
    setError(QStringLiteral("Settings not saved; fix %1 first").arg(path_));
    return false;
  }
  // Edit the existing document so unknown keys survive; comments do not.
  toml::table table;
  if (!isAbsent(path_)) {
    QFile file(path_);
    if (!file.open(QIODevice::ReadOnly)) {
      setError(QStringLiteral("Cannot read %1: %2").arg(path_, file.errorString()));
      return false;
    }
    try {
      const QByteArray bytes = file.readAll();
      if (file.error() != QFileDevice::NoError) {
        setError(QStringLiteral("Cannot read %1: %2").arg(path_, file.errorString()));
        return false;
      }
      table = toml::parse(std::string_view(bytes.constData(), static_cast<std::size_t>(bytes.size())));
    } catch (const toml::parse_error&) {
      readable_ = false;
      setError(QStringLiteral("Settings not saved; %1 was changed and is not valid TOML").arg(path_));
      return false;
    }
  }
  if (!table["reader"].is_table()) {
    table.insert_or_assign("reader", toml::table{});
  }
  auto& reader = *table["reader"].as_table();
  reader.insert_or_assign("zoom", values_.zoom);
  reader.insert_or_assign("column_width",
                          std::string(std::ranges::find(kWidthNames, values_.width_preset, &WidthName::value)->name));
  reader.insert_or_assign("theme", std::string(std::ranges::find(kThemeNames, values_.theme, &ThemeName::value)->name));
  reader.insert_or_assign("body_font", values_.body_font.toStdString());
  reader.insert_or_assign("code_font", values_.code_font.toStdString());
  reader.insert_or_assign("block_spacing", values_.block_spacing);
  reader.insert_or_assign("link_underline", values_.link_underline);
  std::ostringstream out;
  out << table << '\n';
  const std::string text = out.str();

  if (!QDir().mkpath(QFileInfo(path_).absolutePath())) {
    setError(QStringLiteral("Cannot create configuration directory for %1").arg(path_));
    return false;
  }
  QSaveFile target(path_);
  if (!target.open(QIODevice::WriteOnly) ||
      target.write(text.data(), static_cast<qint64>(text.size())) != static_cast<qint64>(text.size()) ||
      !target.commit()) {
    setError(QStringLiteral("Cannot save %1: %2").arg(path_, target.errorString()));
    return false;
  }
  setError({});
  return true;
}
