#include "navigationcontroller.h"

#include <algorithm>

NavigationController::NavigationController(QObject* parent) : QObject(parent), session_(this) {
  connect(&session_, &DocumentSession::documentChanged, this, &NavigationController::committed);
  connect(&session_, &DocumentSession::stateChanged, this, [this] {
    if (session_.state() == DocumentSession::State::Loading) {
      error_.clear();
    }
    if (session_.state() == DocumentSession::State::Failed) {
      pending_ = QUrl();
      pending_index_ = -1;
    }
    emit changed();
  });
}
QUrl NavigationController::imageRoot() const {
  return session_.url().isLocalFile()
             ? QUrl::fromLocalFile(QFileInfo(session_.url().toLocalFile()).absolutePath() + QLatin1Char('/'))
             : QUrl();
}
void NavigationController::setPosition(double fraction) {
  if (session_.state() == DocumentSession::State::Loading) {
    return;
  }
  position_ = std::clamp(fraction, 0.0, 1.0);
  if (applying_) {
    return;
  }
  if (index_ >= 0) {
    history_[index_].fraction = position_;
  }
}
void NavigationController::openPath(const QString& path) {
  const auto resolved = DocumentSession::resolvePath(path);
  if (resolved) {
    open(QUrl::fromLocalFile(*resolved));
  } else {
    error_ = resolved.error();
    emit errorOccurred(error_);
    emit navigating();
    session_.openPath(path);
    emit changed();
  }
}
void NavigationController::setLauncher(std::function<bool(const QUrl&)> launcher) { launcher_ = std::move(launcher); }
void NavigationController::open(const QUrl& url) { visit(url); }
void NavigationController::activate(const QString& destination) {
  const QUrl url = session_.url().resolved(QUrl(destination));
  if (session_.revision() == 0 && destination.startsWith(QLatin1Char('#'))) {
    error_.clear();
    emit navigating();
    emit restoreRequested(0, url.fragment(QUrl::FullyEncoded), false, false);
    emit changed();
    return;
  }
  if ((url.scheme() == QLatin1String("http") || url.scheme() == QLatin1String("https")) && url.isValid() &&
      !url.host().isEmpty()) {
    error_ = launcher_(url) ? QString() : tr("Cannot launch browser: %1").arg(url.toString());
    if (!error_.isEmpty()) {
      emit errorOccurred(error_);
    }
    emit changed();
    return;
  }
  const QString suffix = QFileInfo(url.path()).suffix().toLower();
  if (url.isLocalFile() && (suffix == QLatin1String("md") || suffix == QLatin1String("markdown"))) {
    visit(url);
    return;
  }
  error_ = tr("Unsupported destination: %1").arg(destination);
  emit errorOccurred(error_);
  emit changed();
}
void NavigationController::visit(const QUrl& url, int target) {
  error_.clear();
  pending_ = url;
  pending_index_ = target;
  const QUrl file = url.adjusted(QUrl::RemoveFragment);
  const bool currentFragment =
      target < 0 && url.hasFragment() && file == session_.url() && session_.state() != DocumentSession::State::Loading;
  if (!currentFragment) {
    emit navigating();
  }
  if ((url.hasFragment() || target >= 0) && file == session_.url() &&
      session_.state() != DocumentSession::State::Loading) {
    // Fragment validation is synchronous in the view; commit only after it accepts navigation.
    applying_ = true;
    emit restoreRequested(target >= 0 ? history_[target].fraction : 0,
                          target >= 0 ? QString() : url.fragment(QUrl::FullyEncoded), false, false);
    applying_ = false;
    if (!error_.isEmpty()) {
      pending_ = QUrl();
      return;
    }
    if (currentFragment) {
      emit navigating();
    }
    if (target >= 0) {
      index_ = target;
      position_ = history_[index_].fraction;
    } else {
      history_.resize(index_ + 1);
      history_.push_back({.url = url, .fraction = position_});
      ++index_;
    }
    pending_ = QUrl();
    emit changed();
    return;
  }
  session_.open(file);
}
void NavigationController::committed() {
  if (session_.reloaded()) {
    emit restoreRequested(position_, QString(), true, true);
    return;
  }
  if (pending_index_ >= 0) {
    index_ = pending_index_;
  } else {
    history_.resize(index_ + 1);
    history_.push_back({.url = pending_.isEmpty() ? session_.url() : pending_, .fraction = 0});
    ++index_;
  }
  const bool historyNavigation = pending_index_ >= 0;
  const Entry entry = history_[index_];
  pending_ = QUrl();
  pending_index_ = -1;
  emit changed();
  position_ = entry.fraction;
  emit restoreRequested(entry.fraction, historyNavigation ? QString() : entry.url.fragment(QUrl::FullyEncoded), false,
                        true);
}
void NavigationController::missingFragment(const QString& fragment) {
  error_ = tr("Heading not found: %1").arg(fragment);
  emit errorOccurred(error_);
  emit changed();
}
void NavigationController::back() {
  if (canGoBack()) {
    visit(history_[index_ - 1].url, index_ - 1);
  }
}
void NavigationController::forward() {
  if (canGoForward()) {
    visit(history_[index_ + 1].url, index_ + 1);
  }
}
