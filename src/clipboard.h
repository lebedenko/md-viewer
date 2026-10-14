#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

class Clipboard final : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

 public:
  explicit Clipboard(QObject* parent = nullptr);
  Q_INVOKABLE static void copyText(const QString& text);
};
