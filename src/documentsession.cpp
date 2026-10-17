#include "documentsession.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QStringConverter>
#include <QtConcurrent/QtConcurrentRun>

namespace {

std::expected<DocumentSession::Decoded, QString> readFile(const QString& path) {
  const QFileInfo info(path);
  if (!info.exists()) {
    return std::unexpected(DocumentSession::tr("File not found: %1").arg(path));
  }
  if (!info.isFile()) {
    return std::unexpected(DocumentSession::tr("Not a file: %1").arg(path));
  }
  if (info.size() > DocumentSession::kMaxBytes) {
    return std::unexpected(DocumentSession::tr("File is larger than %1 MiB: %2")
                               .arg(DocumentSession::kMaxBytes / (1024 * 1024))
                               .arg(path));
  }
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    return std::unexpected(DocumentSession::tr("Cannot read %1: %2").arg(path, file.errorString()));
  }
  // Read one byte past the cap so a file that grows after the size check is still rejected.
  const QByteArray bytes = file.read(DocumentSession::kMaxBytes + 1);
  if (bytes.size() > DocumentSession::kMaxBytes) {
    return std::unexpected(DocumentSession::tr("File is larger than %1 MiB: %2")
                               .arg(DocumentSession::kMaxBytes / (1024 * 1024))
                               .arg(path));
  }
  if (file.error() != QFileDevice::NoError) {
    return std::unexpected(DocumentSession::tr("Cannot read %1: %2").arg(path, file.errorString()));
  }
  return DocumentSession::decode(bytes);
}

}  // namespace

DocumentSession::DocumentSession(QObject* parent) : QObject(parent) {
  reload_timer_.setSingleShot(true);
  reload_timer_.setInterval(kReloadDebounceMs);
  connect(&watcher_, &QFileSystemWatcher::fileChanged, &reload_timer_, qOverload<>(&QTimer::start));
  connect(&watcher_, &QFileSystemWatcher::directoryChanged, &reload_timer_, qOverload<>(&QTimer::start));
  connect(&reload_timer_, &QTimer::timeout, this, [this] {
    const QString path = url_.toLocalFile();
    if (!path.isEmpty()) {
      load(path, true);
    }
  });
}

std::expected<QString, QString> DocumentSession::resolvePath(const QString& input) {
  QString path = input;
  if (path.isEmpty()) {
    return std::unexpected(tr("No file path given"));
  }
  if (path.startsWith(QStringLiteral("file:"), Qt::CaseInsensitive)) {
    const QUrl url(path);
    if (!url.isLocalFile()) {
      return std::unexpected(tr("Only local files can be opened: %1").arg(input));
    }
    path = url.toLocalFile();
  } else if (path.contains(QStringLiteral("://"))) {
    return std::unexpected(tr("Only local files can be opened: %1").arg(input));
  } else if (path == QLatin1String("~") || path.startsWith(QLatin1String("~/"))) {
    path = QDir::homePath() + path.mid(1);
  } else if (path.startsWith(QLatin1Char('~'))) {
    return std::unexpected(tr("Home directories of other users are not supported: %1").arg(input));
  }
  return QDir::current().absoluteFilePath(path);
}

QString DocumentSession::abbreviateHome(const QString& path) {
  const QString home = QDir::cleanPath(QDir::homePath());
  if (home.isEmpty() || home == QLatin1String("/")) {
    return path;
  }
  if (path == home) {
    return QStringLiteral("~");
  }
  return path.startsWith(home + QLatin1Char('/')) ? QLatin1Char('~') + path.mid(home.size()) : path;
}

std::expected<DocumentSession::Decoded, QString> DocumentSession::decode(const QByteArray& bytes) {
  const bool utf32 =
      bytes.startsWith(QByteArray("\xFF\xFE\x00\x00", 4)) || bytes.startsWith(QByteArray("\x00\x00\xFE\xFF", 4));
  const bool utf16 = bytes.startsWith("\xFF\xFE") || bytes.startsWith("\xFE\xFF");
  if (utf32 || utf16) {
    return std::unexpected(tr("Unsupported encoding (UTF-16/UTF-32); only UTF-8 is supported"));
  }
  const QByteArray body = bytes.startsWith("\xEF\xBB\xBF") ? bytes.mid(3) : bytes;
  QStringDecoder decoder(QStringConverter::Utf8, QStringConverter::Flag::Stateless);
  Decoded result;
  result.text = decoder(body);
  result.lossy = decoder.hasError();
  return result;
}

void DocumentSession::open(const QUrl& url) {
  if (!url.isLocalFile()) {
    fail(tr("Only local files can be opened: %1").arg(url.toString()));
    return;
  }
  load(QDir::current().absoluteFilePath(url.toLocalFile()));
}

void DocumentSession::openPath(const QString& path) {
  const auto resolved = resolvePath(path);
  if (!resolved) {
    fail(resolved.error());
    return;
  }
  load(*resolved);
}

void DocumentSession::load(const QString& path, bool reload) {
  const quint64 generation = ++generation_;
  state_ = State::Loading;
  error_.clear();
  emit stateChanged();
  auto* watcher = new QFutureWatcher<std::expected<Decoded, QString>>(this);
  connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher, generation, path, reload] {
    const auto result = watcher->result();
    watcher->deleteLater();
    if (generation != generation_) {
      return;  // A newer request superseded this one.
    }
    if (reload) {
      watch(path);  // An atomic replace drops the file watch; the directory watch survives.
    }
    if (!result) {
      fail(result.error());
      return;
    }
    const QString warning = result->lossy ? tr("Invalid UTF-8 bytes were replaced") : QString();
    if (reload && result->text == markdown_ && warning == warning_) {
      state_ = State::Ready;
      error_.clear();
      emit stateChanged();
      return;
    }
    markdown_ = result->text;
    warning_ = warning;
    reloaded_ = reload;
    url_ = QUrl::fromLocalFile(path);
    watch(path);
    title_ = QFileInfo(path).fileName();
    ++revision_;
    state_ = State::Ready;
    error_.clear();
    emit documentChanged();
    emit stateChanged();
  });
  watcher->setFuture(QtConcurrent::run([path] { return readFile(path); }));
}

void DocumentSession::watch(const QString& path) {
  const QString dir = QFileInfo(path).absolutePath();
  if (!watcher_.files().contains(path) && QFileInfo::exists(path)) {
    watcher_.addPath(path);
  }
  if (!watcher_.directories().contains(dir)) {
    watcher_.addPath(dir);
  }
  const QStringList stale = watcher_.files() + watcher_.directories();
  for (const QString& entry : stale) {
    if (entry != path && entry != dir) {
      watcher_.removePath(entry);
    }
  }
}

void DocumentSession::fail(const QString& message) {
  ++generation_;  // A failure also supersedes any load in flight.
  state_ = State::Failed;
  error_ = message;
  emit stateChanged();
}
