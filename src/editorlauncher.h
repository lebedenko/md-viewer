#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

#include <expected>

// Opens the current local document in the configured editor (argv, never a shell) or the desktop's default handler.
class EditorLauncher : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QStringList command READ command WRITE setCommand NOTIFY commandChanged)
  Q_PROPERTY(QString error READ error NOTIFY errorChanged)

 public:
  struct Invocation {
    QString program;
    QStringList arguments;
  };

  // "{file}" is replaced inside each element (never re-split); without it the path is appended.
  static std::expected<Invocation, QString> buildInvocation(const QStringList& command, const QString& file);

  explicit EditorLauncher(QObject* parent = nullptr);

  [[nodiscard]] QStringList command() const { return command_; }
  void setCommand(const QStringList& command);
  [[nodiscard]] QString error() const { return error_; }
  Q_INVOKABLE bool openInEditor(const QUrl& file);

 signals:
  void errorOccurred(const QString& message);
  void commandChanged();
  void errorChanged();

 private:
  void setError(const QString& error);

  QStringList command_;
  QString error_;
};
