#include "appconfig.h"

#include "readersettings.h"

#include <QColor>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <optional>
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
  struct stat info = {};
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

bool applyBoolean(std::string_view name, const toml::node& node, AppConfig::Values& values) {
  using Member = bool AppConfig::Values::*;
  struct Preference {
    std::string_view name;
    Member member;
  };
  static constexpr std::array preferences = {
      Preference{.name = "math_enabled", .member = &AppConfig::Values::math_enabled},
      Preference{.name = "highlighting_enabled", .member = &AppConfig::Values::highlighting_enabled},
      Preference{.name = "code_wrapping", .member = &AppConfig::Values::code_wrapping},
      Preference{.name = "line_numbers", .member = &AppConfig::Values::line_numbers},
      Preference{.name = "height_limiting", .member = &AppConfig::Values::height_limiting},
      Preference{.name = "link_underline", .member = &AppConfig::Values::link_underline}};
  const auto* found = std::ranges::find(preferences, name, &Preference::name);
  if (found == preferences.end()) {
    return false;
  }
  if (node.is_boolean()) {
    values.*found->member = *node.value<bool>();
  } else {
    values.warnings.append(QStringLiteral("reader.%1 must be true or false").arg(toQString(name)));
  }
  return true;
}

std::optional<QVariant> validatedSetting(const QVariantMap& field, const QVariant& value) {
  const QString kind = field.value("kind").toString();
  if (kind == "color") {
    const QColor color(value.toString());
    if (color.isValid()) {
      return color.name(QColor::HexArgb);
    }
  } else if (kind == "font") {
    QFont font;
    if (value.metaType() == QMetaType::fromType<QFont>()) {
      font = value.value<QFont>();
    } else if (value.metaType() != QMetaType::fromType<QString>() || value.toString().count(',') < 9 ||
               !font.fromString(value.toString())) {
      return {};
    }
    if (!font.family().isEmpty() && font.pointSizeF() > 0) {
      return font;
    }
  } else if (kind == "bool") {
    if (value.metaType() == QMetaType::fromType<bool>()) {
      return value;
    }
  } else if (value.metaType() != QMetaType::fromType<QString>() && value.metaType() != QMetaType::fromType<bool>() &&
             value.canConvert<double>()) {
    const double number = value.toDouble();
    if (std::isfinite(number) && number >= field.value("minimum").toDouble() &&
        number <= field.value("maximum").toDouble() && (kind == "number" || number == std::floor(number))) {
      return number;
    }
  }
  return {};
}

void parseReaderGroup(const toml::table& table, const QString& prefix, AppConfig::Values& values) {
  for (const auto& [key, node] : table) {
    const QString path = prefix + "." + toQString(key.str());
    if (const auto* group = node.as_table()) {
      const auto fields = AppConfig::settingFields();
      const bool known = std::ranges::any_of(fields, [&path](const QVariant& entry) {
        return entry.toMap().value("key").toString().startsWith(path + '.');
      });
      if (known) {
        parseReaderGroup(*group, path, values);
      } else {
        values.warnings.append("invalid or unknown reader." + path);
      }
      continue;
    }
    QVariant input;
    if (node.is_boolean()) {
      input = *node.value<bool>();
    } else if (node.is_number()) {
      input = *node.value<double>();
    } else if (node.is_string()) {
      input = toQString(*node.value<std::string_view>());
    }
    bool valid = false;
    for (const auto& entry : AppConfig::settingFields()) {
      const auto field = entry.toMap();
      if (field.value("key").toString() != path) {
        continue;
      }
      if (auto normalized = validatedSetting(field, input)) {
        values.reader_settings.insert(path, *normalized);
        valid = true;
      }
      break;
    }
    if (!valid) {
      values.warnings.append("invalid or unknown reader." + path);
    }
  }
}

void saveReaderSettings(toml::table& reader, const QVariantMap& settings) {
  for (const auto& entry : AppConfig::settingFields()) {
    const auto field = entry.toMap();
    const QString key = field.value("key").toString();
    const auto parts = key.split('.');
    toml::table* target = &reader;
    for (qsizetype i = 0; i < parts.size() - 1; ++i) {
      const auto part = parts[i].toStdString();
      if (!(*target)[part].is_table()) {
        target->insert_or_assign(part, toml::table{});
      }
      target = (*target)[part].as_table();
    }
    const auto leaf = parts.last().toStdString();
    if (!settings.contains(key)) {
      target->erase(leaf);
      continue;
    }
    const auto value = settings.value(key);
    const auto kind = field.value("kind").toString();
    if (kind == "bool") {
      target->insert_or_assign(leaf, value.toBool());
    } else if (kind == "font") {
      target->insert_or_assign(leaf, value.value<QFont>().toString().toStdString());
    } else if (kind == "color") {
      target->insert_or_assign(leaf, value.toString().toStdString());
    } else {
      target->insert_or_assign(leaf, value.toDouble());
    }
  }
}

void applyReader(const toml::table& reader, AppConfig::Values& values) {
  for (const auto& [key, node] : reader) {
    const std::string_view name = key.str();
    if (name == "selection_enabled") {
      continue;
    }
    if (applyBoolean(name, node, values)) {
      continue;
    }
    if (name == "options" || name == "style" || name == "syntax" || name == "view") {
      if (const auto* group = node.as_table()) {
        parseReaderGroup(*group, toQString(name), values);
      } else {
        values.warnings.append("reader." + toQString(name) + " must be a table");
      }
    } else if (name == "zoom") {
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
    } else {
      values.warnings.append(QStringLiteral("unknown key 'reader.%1'").arg(toQString(name)));
    }
  }
}

void applyEditor(const toml::table& editor, AppConfig::Values& values) {
  for (const auto& [key, node] : editor) {
    const std::string_view name = key.str();
    if (name != "command") {
      values.warnings.append(QStringLiteral("unknown key 'editor.%1'").arg(toQString(name)));
      continue;
    }
    const auto* array = node.as_array();
    QStringList command;
    bool valid = array != nullptr && !array->empty();
    for (std::size_t i = 0; valid && i < array->size(); ++i) {
      const auto text = array->get(i)->value<std::string_view>();
      valid = text && !text->empty();
      if (valid) {
        command.append(toQString(*text));
      }
    }
    if (valid) {
      values.editor_command = command;
    } else {
      values.warnings.append(QStringLiteral("editor.command must be a non-empty array of non-empty strings"));
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
    if (key.str() == "header") {
      if (!node.is_table()) {
        values.warnings.append(QStringLiteral("'header' must be a table"));
      }
    } else if (key.str() == "editor") {
      if (!node.is_table()) {
        values.warnings.append(QStringLiteral("'editor' must be a table"));
      }
    } else if (key.str() != "reader") {
      values.warnings.append(
          QStringLiteral("unknown key '%1'").arg(QString::fromUtf8(key.data(), static_cast<qsizetype>(key.length()))));
    } else if (!node.is_table()) {
      values.warnings.append(QStringLiteral("'reader' must be a table"));
    }
  }
  if (const auto* reader = table["reader"].as_table()) {
    applyReader(*reader, values);
  }
  if (const auto* editor = table["editor"].as_table()) {
    applyEditor(*editor, values);
  }
  const int minimum = values.reader_settings.value("options.codeBlockMinimumLines", 8).toInt();
  const int maximum = values.reader_settings.value("options.codeBlockMaximumLines", 30).toInt();
  if (maximum < minimum) {
    values.warnings.append("reader.options.codeBlockMaximumLines must be at least minimum lines");
    values.reader_settings.insert("options.codeBlockMaximumLines", minimum);
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
  const QFileInfo base(path_);
  for (QString token : qEnvironmentVariable("XDG_CURRENT_DESKTOP").split(':', Qt::SkipEmptyParts)) {
    bool valid = true;
    for (QChar& character : token) {
      const ushort code = character.unicode();
      if (code >= 'A' && code <= 'Z') {
        character = QChar(code + ('a' - 'A'));
      } else if ((code < 'a' || code > 'z') && (code < '0' || code > '9') && code != '_' && code != '-') {
        valid = false;
      }
    }
    if (!valid) {
      qWarning().noquote() << "Configuration: invalid desktop token" << token;
      continue;
    }
    desktop_paths_.append(base.dir().filePath(base.completeBaseName() + '-' + token + ".toml"));
  }
  header_timer_.setSingleShot(true);
  header_timer_.setInterval(100);
  connect(&header_timer_, &QTimer::timeout, this, &AppConfig::reloadHeader);
  connect(&header_watcher_, &QFileSystemWatcher::fileChanged, this, [this] { header_timer_.start(); });
  connect(&header_watcher_, &QFileSystemWatcher::directoryChanged, this, [this] { header_timer_.start(); });
  reloadHeader();
}

void AppConfig::watchHeader() {
  if (!header_watcher_.files().isEmpty()) {
    header_watcher_.removePaths(header_watcher_.files());
  }
  if (!header_watcher_.directories().isEmpty()) {
    header_watcher_.removePaths(header_watcher_.directories());
  }
  QStringList paths = desktop_paths_;
  paths.prepend(path_);
  QStringList watches;
  for (const auto& path : paths) {
    if (QFileInfo::exists(path)) {
      watches.append(path);
    }
    QDir directory = QFileInfo(path).dir();
    while (true) {
      if (directory.exists()) {
        watches.append(directory.absolutePath());
      }
      if (!directory.cdUp()) {
        break;
      }
    }
  }
  watches.removeDuplicates();
  if (!watches.isEmpty()) {
    header_watcher_.addPaths(watches);
  }
}

void AppConfig::reloadHeader() {
  bool title = true;
  bool close = true;
  const auto apply = [&title, &close](const QString& path) {
    QFile file(path);
    if (!file.exists()) {
      return;
    }
    if (!file.open(QIODevice::ReadOnly)) {
      qWarning().noquote() << "Configuration: cannot read" << path;
      return;
    }
    const auto bytes = file.readAll();
    if (file.error() != QFileDevice::NoError) {
      return;
    }
    try {
      const auto table = toml::parse(std::string_view(bytes.constData(), static_cast<std::size_t>(bytes.size())));
      for (const auto& entry : {std::pair{"show_title", &title}, std::pair{"show_close_button", &close}}) {
        const auto node = table["header"][entry.first];
        if (node.is_boolean()) {
          *entry.second = *node.value<bool>();
        } else if (node) {
          qWarning().noquote() << "Configuration" << path << "- header." << entry.first << "must be boolean";
        }
      }
    } catch (const toml::parse_error&) {
      qWarning().noquote() << "Configuration: malformed header file" << path;
    }
  };
  apply(path_);
  for (const auto& path : desktop_paths_) {
    if (!isAbsent(path)) {
      apply(path);
      break;
    }
  }
  if (title != show_title_ || close != show_close_button_) {
    show_title_ = title;
    show_close_button_ = close;
    emit headerChanged();
  }
  watchHeader();
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

void AppConfig::setMathEnabled(bool value) {
  if (value == values_.math_enabled) {
    return;
  }
  values_.math_enabled = value;
  emit mathEnabledChanged();
  scheduleSave();
}

void AppConfig::setHighlightingEnabled(bool value) {
  if (value == values_.highlighting_enabled) {
    return;
  }
  values_.highlighting_enabled = value;
  emit highlightingEnabledChanged();
  scheduleSave();
}

void AppConfig::setCodeWrapping(bool value) {
  if (value == values_.code_wrapping) {
    return;
  }
  values_.code_wrapping = value;
  emit codeWrappingChanged();
  scheduleSave();
}

void AppConfig::setLineNumbers(bool value) {
  if (value == values_.line_numbers) {
    return;
  }
  values_.line_numbers = value;
  emit lineNumbersChanged();
  scheduleSave();
}

void AppConfig::setHeightLimiting(bool value) {
  if (value == values_.height_limiting) {
    return;
  }
  values_.height_limiting = value;
  emit heightLimitingChanged();
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
  reader.erase("selection_enabled");
  saveReaderSettings(reader, values_.reader_settings);
  reader.insert_or_assign("math_enabled", values_.math_enabled);
  reader.insert_or_assign("highlighting_enabled", values_.highlighting_enabled);
  reader.insert_or_assign("code_wrapping", values_.code_wrapping);
  reader.insert_or_assign("line_numbers", values_.line_numbers);
  reader.insert_or_assign("height_limiting", values_.height_limiting);
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

QVariantList AppConfig::settingFields() {
  static const QVariantList fields = QJsonDocument::fromJson(kReaderSettings).array().toVariantList();
  return fields;
}
QVariant AppConfig::readerSetting(const QString& key) const {
  if (values_.reader_settings.contains(key)) {
    return values_.reader_settings.value(key);
  }
  for (const auto& entry : settingFields()) {
    const auto field = entry.toMap();
    if (field.value("key").toString() == key) {
      return field.value("defaultValue");
    }
  }
  return {};
}
void AppConfig::setReaderSetting(const QString& key, const QVariant& value) {
  for (const auto& entry : settingFields()) {
    const auto field = entry.toMap();
    if (field.value("key").toString() != key) {
      continue;
    }
    const auto normalized = validatedSetting(field, value);
    if (!normalized || values_.reader_settings.value(key) == *normalized) {
      return;
    }
    if (key == "options.codeBlockMinimumLines" &&
        normalized->toInt() > readerSetting("options.codeBlockMaximumLines").toInt()) {
      return;
    }
    if (key == "options.codeBlockMaximumLines" &&
        normalized->toInt() < readerSetting("options.codeBlockMinimumLines").toInt()) {
      return;
    }
    values_.reader_settings.insert(key, *normalized);
    emit readerSettingsChanged();
    scheduleSave();
    return;
  }
}
void AppConfig::resetReaderSetting(const QString& key) {
  if (values_.reader_settings.remove(key) == 0) {
    return;
  }
  if (key == "options.codeBlockMaximumLines" && readerSetting("options.codeBlockMinimumLines").toInt() > 30) {
    values_.reader_settings.remove("options.codeBlockMinimumLines");
  }
  if (key == "options.codeBlockMinimumLines" && readerSetting("options.codeBlockMaximumLines").toInt() < 8) {
    values_.reader_settings.remove("options.codeBlockMaximumLines");
  }
  emit readerSettingsChanged();
  scheduleSave();
}
