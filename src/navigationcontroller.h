#pragma once

#include "documentsession.h"

#include <QDesktopServices>
#include <QFileInfo>

#include <functional>
#include <vector>

class NavigationController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(DocumentSession* session READ session CONSTANT)
  Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY changed)
  Q_PROPERTY(bool canGoForward READ canGoForward NOTIFY changed)
  Q_PROPERTY(QString error READ error NOTIFY changed)
  Q_PROPERTY(QUrl imageRoot READ imageRoot NOTIFY changed)
 public:
  explicit NavigationController(QObject* parent = nullptr);
  DocumentSession* session() { return &session_; }
  [[nodiscard]] bool canGoBack() const { return index_ > 0; }
  [[nodiscard]] bool canGoForward() const { return index_ + 1 < static_cast<int>(history_.size()); }
  [[nodiscard]] QString error() const { return error_; }
  [[nodiscard]] QUrl imageRoot() const;
  Q_INVOKABLE void open(const QUrl& url);
  Q_INVOKABLE void openPath(const QString& path);
  Q_INVOKABLE void activate(const QString& destination);
  Q_INVOKABLE void back();
  Q_INVOKABLE void forward();
  Q_INVOKABLE void setPosition(double fraction);
  Q_INVOKABLE void missingFragment(const QString& fragment);
  void setLauncher(std::function<bool(const QUrl&)> launcher);
 signals:
  void changed();
  void navigating();
  void restoreRequested(double fraction, const QString& fragment, bool reload, bool newDocument);

 private:
  struct Entry {
    QUrl url;
    double fraction = 0;
  };
  void visit(const QUrl& url, int target = -1);
  void committed();
  std::function<bool(const QUrl&)> launcher_ = QDesktopServices::openUrl;
  DocumentSession session_;
  std::vector<Entry> history_;
  int index_ = -1;
  int pending_index_ = -1;
  QUrl pending_;
  QString error_;
  double position_ = 0;
  bool applying_ = false;
};
