#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

#include <expected>

// TOML application configuration. The file is read once at construction; changes are written through immediately.
class AppConfig : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(double zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
  Q_PROPERTY(int widthPreset READ widthPreset WRITE setWidthPreset NOTIFY widthPresetChanged)
  Q_PROPERTY(QString path READ path CONSTANT)
  Q_PROPERTY(QString error READ error NOTIFY errorChanged)

 public:
  static constexpr double kMinZoom = 0.5;
  static constexpr double kMaxZoom = 3.0;
  static constexpr int kWidthPresetCount = 3;  // narrow, comfortable, wide

  struct Values {
    double zoom = 1.0;
    int width_preset = 1;
    QStringList warnings;  // unknown keys and invalid values; the file is still usable
  };

  // $XDG_CONFIG_HOME/md-viewer/config.toml (absolute XDG only), then ~/.config/md-viewer.toml.
  static QStringList candidatePaths(const QString& xdg_config_home, const QString& home);
  // Wrong-typed or unknown values fall back per key (with a warning); only a TOML syntax error fails.
  static std::expected<Values, QString> parse(const QByteArray& bytes);

  explicit AppConfig(QObject* parent = nullptr);
  explicit AppConfig(const QStringList& candidates, QObject* parent = nullptr);

  [[nodiscard]] double zoom() const { return values_.zoom; }
  [[nodiscard]] int widthPreset() const { return values_.width_preset; }
  [[nodiscard]] QString path() const { return path_; }
  [[nodiscard]] QString error() const { return error_; }
  void setZoom(double zoom);
  void setWidthPreset(int preset);

 signals:
  void zoomChanged();
  void widthPresetChanged();
  void errorChanged();

 private:
  void load(const QStringList& candidates);
  void save();
  void setError(const QString& error);

  Values values_;
  QString path_;
  QString error_;
  bool readable_ = true;  // false after a syntax error: the file is never overwritten
};
