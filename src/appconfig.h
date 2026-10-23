#pragma once

#include <QByteArray>
#include <QFileSystemWatcher>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

#include <expected>

// TOML reader preferences persist after a short debounce; desktop header preferences also reload live.
class AppConfig : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(AppConfig)
  QML_ELEMENT
  Q_PROPERTY(bool showTitle READ showTitle NOTIFY headerChanged)
  Q_PROPERTY(bool showCloseButton READ showCloseButton NOTIFY headerChanged)
  Q_PROPERTY(double zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
  Q_PROPERTY(int widthPreset READ widthPreset WRITE setWidthPreset NOTIFY widthPresetChanged)
  Q_PROPERTY(int theme READ theme WRITE setTheme NOTIFY themeChanged)
  Q_PROPERTY(QString bodyFont READ bodyFont WRITE setBodyFont NOTIFY bodyFontChanged)
  Q_PROPERTY(QString codeFont READ codeFont WRITE setCodeFont NOTIFY codeFontChanged)
  Q_PROPERTY(double blockSpacing READ blockSpacing WRITE setBlockSpacing NOTIFY blockSpacingChanged)
  Q_PROPERTY(bool linkUnderline READ linkUnderline WRITE setLinkUnderline NOTIFY linkUnderlineChanged)
  Q_PROPERTY(bool selectionEnabled READ selectionEnabled WRITE setSelectionEnabled NOTIFY selectionEnabledChanged)
  Q_PROPERTY(bool mathEnabled READ mathEnabled WRITE setMathEnabled NOTIFY mathEnabledChanged)
  Q_PROPERTY(
      bool highlightingEnabled READ highlightingEnabled WRITE setHighlightingEnabled NOTIFY highlightingEnabledChanged)
  Q_PROPERTY(bool codeWrapping READ codeWrapping WRITE setCodeWrapping NOTIFY codeWrappingChanged)
  Q_PROPERTY(bool lineNumbers READ lineNumbers WRITE setLineNumbers NOTIFY lineNumbersChanged)
  Q_PROPERTY(bool heightLimiting READ heightLimiting WRITE setHeightLimiting NOTIFY heightLimitingChanged)
  Q_PROPERTY(QStringList editorCommand READ editorCommand CONSTANT)
  Q_PROPERTY(QString path READ path CONSTANT)
  Q_PROPERTY(QString error READ error NOTIFY errorChanged)

 public:
  // Unscoped int values preserve the integer property and the QML AppConfig.Narrow API.
  // NOLINTNEXTLINE(cppcoreguidelines-use-enum-class, performance-enum-size)
  enum WidthPreset : int { Narrow = 0, Comfortable = 1, Wide = 2 };
  Q_ENUM(WidthPreset)

  // NOLINTNEXTLINE(cppcoreguidelines-use-enum-class, performance-enum-size)
  enum Theme : int { System = 0, Light = 1, Dark = 2 };
  Q_ENUM(Theme)

  static constexpr double kMinZoom = 0.5;
  static constexpr double kMaxZoom = 3.0;
  static constexpr int kWidthPresetCount = 3;  // narrow, comfortable, wide
  static constexpr int kThemeCount = 3;        // system, light, dark
  static constexpr double kMinBlockSpacing = 0.5;
  static constexpr double kMaxBlockSpacing = 2.0;

  struct Values {
    double zoom = 1.0;
    int width_preset = Comfortable;
    int theme = System;
    QString body_font;  // empty: the style preset's family
    QString code_font;
    double block_spacing = 1.0;  // multiplier of the preset's block spacing
    bool link_underline = true;
    bool selection_enabled = true;
    bool math_enabled = true;
    bool highlighting_enabled = true;
    bool code_wrapping = false;
    bool line_numbers = true;
    bool height_limiting = true;
    QStringList editor_command;  // argv with optional {file}; empty: the desktop's default handler
    QStringList warnings;        // unknown keys and invalid values; the file is still usable
  };

  // $XDG_CONFIG_HOME/md-viewer/config.toml (absolute XDG only), then ~/.config/md-viewer.toml.
  static QStringList candidatePaths(const QString& xdg_config_home, const QString& home);
  // Wrong-typed or unknown values fall back per key (with a warning); only a TOML syntax error fails.
  static std::expected<Values, QString> parse(const QByteArray& bytes);

  explicit AppConfig(QObject* parent = nullptr);
  explicit AppConfig(const QStringList& candidates, QObject* parent = nullptr);

  ~AppConfig() override;
  bool flush();

  [[nodiscard]] bool showTitle() const { return show_title_; }
  [[nodiscard]] bool showCloseButton() const { return show_close_button_; }
  [[nodiscard]] double zoom() const { return values_.zoom; }
  [[nodiscard]] int widthPreset() const { return values_.width_preset; }
  [[nodiscard]] int theme() const { return values_.theme; }
  [[nodiscard]] QString bodyFont() const { return values_.body_font; }
  [[nodiscard]] QString codeFont() const { return values_.code_font; }
  [[nodiscard]] double blockSpacing() const { return values_.block_spacing; }
  [[nodiscard]] bool linkUnderline() const { return values_.link_underline; }
  [[nodiscard]] bool selectionEnabled() const { return values_.selection_enabled; }
  void setSelectionEnabled(bool value);
  [[nodiscard]] bool mathEnabled() const { return values_.math_enabled; }
  void setMathEnabled(bool value);
  [[nodiscard]] bool highlightingEnabled() const { return values_.highlighting_enabled; }
  void setHighlightingEnabled(bool value);
  [[nodiscard]] bool codeWrapping() const { return values_.code_wrapping; }
  void setCodeWrapping(bool value);
  [[nodiscard]] bool lineNumbers() const { return values_.line_numbers; }
  void setLineNumbers(bool value);
  [[nodiscard]] bool heightLimiting() const { return values_.height_limiting; }
  void setHeightLimiting(bool value);
  [[nodiscard]] QStringList editorCommand() const { return values_.editor_command; }
  [[nodiscard]] QString path() const { return path_; }
  [[nodiscard]] QString error() const { return error_; }
  void setZoom(double zoom);
  void setWidthPreset(int preset);
  void setTheme(int theme);
  void setBodyFont(const QString& family);
  void setCodeFont(const QString& family);
  void setBlockSpacing(double spacing);
  void setLinkUnderline(bool underline);

 signals:
  void headerChanged();
  void zoomChanged();
  void widthPresetChanged();
  void themeChanged();
  void bodyFontChanged();
  void codeFontChanged();
  void blockSpacingChanged();
  void linkUnderlineChanged();
  void selectionEnabledChanged();
  void mathEnabledChanged();
  void highlightingEnabledChanged();
  void codeWrappingChanged();
  void lineNumbersChanged();
  void heightLimitingChanged();
  void errorChanged();

 private:
  void reloadHeader();
  void watchHeader();
  void load(const QStringList& candidates);
  bool save();
  void scheduleSave();
  void setError(const QString& error);

  QFileSystemWatcher header_watcher_;
  QTimer header_timer_;
  QStringList desktop_paths_;
  bool show_title_ = true;
  bool show_close_button_ = true;
  QTimer save_timer_;
  bool pending_ = false;
  Values values_;
  QString path_;
  QString error_;
  bool readable_ = true;  // false after a syntax error: the file is never overwritten
};
