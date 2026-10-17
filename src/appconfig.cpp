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

bool isAbsent(const QString& path) {
  struct stat info{};
  return ::lstat(QFile::encodeName(path).constData(), &info) != 0 && errno == ENOENT;
}

void applyReader(const toml::table& reader, AppConfig::Values& values) {
  for (const auto& [key, node] : reader) {
    const std::string_view name = key.str();
    if (name == "zoom") {
      const auto zoom = node.value<double>();
      if (zoom && std::isfinite(*zoom)) {
        values.zoom = std::clamp(*zoom, AppConfig::kMinZoom, AppConfig::kMaxZoom);
      } else {
        values.warnings.append(QStringLiteral("reader.zoom must be a number"));
      }
    } else if (name == "column_width") {
      const auto text = node.value<std::string_view>();
      const auto* const found = text ? std::ranges::find(kWidthNames, *text, &WidthName::name) : kWidthNames.end();
      if (found != kWidthNames.end()) {
        values.width_preset = found->value;
      } else {
        values.warnings.append(QStringLiteral("reader.column_width must be \"narrow\", \"comfortable\" or \"wide\""));
      }
    } else {
      values.warnings.append(QStringLiteral("unknown key 'reader.%1'")
                                 .arg(QString::fromUtf8(name.data(), static_cast<qsizetype>(name.size()))));
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
