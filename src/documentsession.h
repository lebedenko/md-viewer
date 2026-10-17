#pragma once

#include <QFileSystemWatcher>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

#include <expected>

class DocumentSession : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(State state READ state NOTIFY stateChanged)
  Q_PROPERTY(QString error READ error NOTIFY stateChanged)
  Q_PROPERTY(QUrl url READ url NOTIFY documentChanged)
  Q_PROPERTY(QString displayPath READ displayPath NOTIFY documentChanged)
  Q_PROPERTY(QString title READ title NOTIFY documentChanged)
  Q_PROPERTY(QString markdown READ markdown NOTIFY documentChanged)
  Q_PROPERTY(QString warning READ warning NOTIFY documentChanged)
  Q_PROPERTY(int revision READ revision NOTIFY documentChanged)
  Q_PROPERTY(bool reloaded READ reloaded NOTIFY documentChanged)

 public:
  enum class State : quint8 { Empty, Loading, Ready, Failed };
  Q_ENUM(State)

  static constexpr qint64 kMaxBytes = 16 * 1024 * 1024;
  static constexpr int kReloadDebounceMs = 150;

  struct Decoded {
    QString text;
    bool lossy = false;
  };

  explicit DocumentSession(QObject* parent = nullptr);

  // Expands ~, makes the path absolute and accepts file: URLs. Pure; touches no file.
  static std::expected<QString, QString> resolvePath(const QString& input);
  // UTF-8 only: strips a BOM, rejects UTF-16/32 marks, replaces invalid bytes with U+FFFD (lossy).
  // Replaces a leading home directory with ~ for display.
  static QString abbreviateHome(const QString& path);
  static std::expected<Decoded, QString> decode(const QByteArray& bytes);

  Q_INVOKABLE void open(const QUrl& url);
  Q_INVOKABLE void openPath(const QString& path);

  [[nodiscard]] State state() const { return state_; }
  [[nodiscard]] QString error() const { return error_; }
  [[nodiscard]] QUrl url() const { return url_; }
  [[nodiscard]] QString displayPath() const { return abbreviateHome(url_.toLocalFile()); }
  [[nodiscard]] QString title() const { return title_; }
  [[nodiscard]] QString markdown() const { return markdown_; }
  [[nodiscard]] QString warning() const { return warning_; }
  [[nodiscard]] int revision() const { return revision_; }
  [[nodiscard]] bool reloaded() const { return reloaded_; }

 signals:
  void stateChanged();
  void documentChanged();

 private:
  void load(const QString& path, bool reload = false);
  void watch(const QString& path);
  void fail(const QString& message);

  State state_ = State::Empty;
  QString error_;
  QUrl url_;
  QString title_;
  QString markdown_;
  QString warning_;
  int revision_ = 0;
  bool reloaded_ = false;
  QFileSystemWatcher watcher_;
  QTimer reload_timer_;
  quint64 generation_ = 0;
};
