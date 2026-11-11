#include "foldermodel.h"

#include "documentsession.h"

#include <QCollator>
#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QLocale>
#include <QVariantMap>
#include <QtConcurrent/QtConcurrentRun>

#include <algorithm>
#include <utility>

namespace {

struct Walk {
  const std::atomic_bool* cancel = nullptr;
  int max_files = 0;
  int files = 0;
  int directories = 0;
  bool truncated = false;
  QCollator collator;
  QStringList ancestors;  // Canonical paths on the current descent; a link back into them is a cycle.
};

bool cancelled(const Walk& walk) { return walk.cancel != nullptr && walk.cancel->load(std::memory_order_relaxed); }

// Fills `node.children` with Markdown files and the subdirectories that contain some, at any depth.
// NOLINTNEXTLINE(misc-no-recursion): depth is bounded by the path length limit and the cycle check.
void walkDirectory(FolderModel::Node& node, Walk& walk) {
  // Hidden entries and broken links are excluded by the default filter; links are followed.
  const QFileInfoList entries =
      QDir(node.path).entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDir::NoSort);
  for (const QFileInfo& entry : entries) {
    if (cancelled(walk)) {
      return;
    }
    FolderModel::Node child{
        .name = entry.fileName(), .path = entry.absoluteFilePath(), .directory = entry.isDir(), .children = {}};
    if (!child.directory) {
      if (!FolderModel::isMarkdown(child.name)) {
        continue;
      }
      if (walk.files >= walk.max_files) {
        walk.truncated = true;
        continue;
      }
      ++walk.files;
      node.children.push_back(std::move(child));
      continue;
    }
    const QString canonical = entry.canonicalFilePath();
    if (canonical.isEmpty() || walk.ancestors.contains(canonical)) {
      continue;
    }
    if (walk.files >= walk.max_files || ++walk.directories > FolderModel::kMaxDirectories) {
      walk.truncated = true;
      continue;
    }
    walk.ancestors.push_back(canonical);
    walkDirectory(child, walk);
    walk.ancestors.pop_back();
    if (!child.children.empty()) {
      node.children.push_back(std::move(child));
    }
  }
  std::ranges::sort(node.children, [&walk](const FolderModel::Node& left, const FolderModel::Node& right) {
    if (left.directory != right.directory) {
      return left.directory;
    }
    const int order = walk.collator.compare(left.name, right.name);
    return order != 0 ? order < 0 : left.name < right.name;
  });
}

// NOLINTNEXTLINE(misc-no-recursion)
void flatten(const FolderModel::Node& node, qsizetype prefix, FolderModel::Scan& scan) {
  scan.directories.push_back(node.path);
  for (const FolderModel::Node& child : node.children) {
    if (child.directory) {
      flatten(child, prefix, scan);
    } else {
      scan.files.push_back(child.path.mid(prefix));
    }
  }
}

}  // namespace

FolderModel::FolderModel(QObject* parent) : QAbstractListModel(parent) {
  refresh_timer_.setSingleShot(true);
  refresh_timer_.setInterval(kRefreshDebounceMs);
  connect(&watcher_, &QFileSystemWatcher::directoryChanged, &refresh_timer_, qOverload<>(&QTimer::start));
  connect(&refresh_timer_, &QTimer::timeout, this, &FolderModel::refresh);
}

FolderModel::~FolderModel() {
  if (cancel_) {
    cancel_->store(true);
  }
}

bool FolderModel::isMarkdown(const QString& fileName) {
  return fileName.endsWith(QLatin1String(".md"), Qt::CaseInsensitive) ||
         fileName.endsWith(QLatin1String(".markdown"), Qt::CaseInsensitive);
}

FolderModel::Scan FolderModel::scan(const QString& root, const std::atomic_bool* cancel, int maxFiles) {
  Scan result;
  const QFileInfo info(root);
  if (!info.exists()) {
    result.error = tr("Folder not found: %1").arg(root);
    return result;
  }
  if (!info.isDir()) {
    result.error = tr("Not a folder: %1").arg(root);
    return result;
  }
  if (!info.isReadable() || !info.isExecutable()) {
    result.error = tr("Cannot read folder: %1").arg(root);
    return result;
  }
  Walk walk;
  walk.cancel = cancel;
  walk.max_files = maxFiles;
  // The C collator ignores numeric mode, including when selected through C.UTF-8.
  if (walk.collator.locale().language() == QLocale::C) {
    walk.collator.setLocale(QLocale(QLocale::English));
  }
  walk.collator.setNumericMode(true);
  walk.collator.setCaseSensitivity(Qt::CaseInsensitive);
  walk.ancestors.push_back(info.canonicalFilePath());
  result.tree = Node{.name = info.fileName(), .path = root, .directory = true, .children = {}};
  walkDirectory(result.tree, walk);
  result.truncated = walk.truncated;
  flatten(result.tree, root.endsWith(QLatin1Char('/')) ? root.size() : root.size() + 1, result);
  return result;
}

int FolderModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(rows_.size());
}

QVariant FolderModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(rows_.size())) {
    return {};
  }
  const Row& row = rows_[index.row()];
  if (role == Qt::DisplayRole) {
    return row.node->name;
  }
  switch (static_cast<Role>(role)) {
    case Role::NameRole:
      return row.node->name;
    case Role::UrlRole:
      return QUrl::fromLocalFile(row.node->path);
    case Role::DirectoryRole:
      return row.node->directory;
    case Role::DepthRole:
      return row.depth;
    case Role::ExpandedRole:
      return row.node->directory && expanded_.contains(row.node->path);
  }
  return {};
}

QHash<int, QByteArray> FolderModel::roleNames() const {
  return {{static_cast<int>(Role::NameRole), "entryName"},
          {static_cast<int>(Role::UrlRole), "entryUrl"},
          {static_cast<int>(Role::DirectoryRole), "isDirectory"},
          {static_cast<int>(Role::DepthRole), "depth"},
          {static_cast<int>(Role::ExpandedRole), "expanded"}};
}

QString FolderModel::name() const {
  const QString file_name = QFileInfo(root_).fileName();
  return file_name.isEmpty() ? root_ : file_name;
}

QString FolderModel::displayPath() const { return DocumentSession::abbreviateHome(root_); }

QUrl FolderModel::readme() const {
  for (const auto* wanted : {"readme.md", "readme.markdown"}) {
    for (const Node& child : scan_.tree.children) {
      if (!child.directory && child.name.compare(QLatin1String(wanted), Qt::CaseInsensitive) == 0) {
        return QUrl::fromLocalFile(child.path);
      }
    }
  }
  return {};
}

bool FolderModel::isDirectory(const QString& path) {
  const auto resolved = DocumentSession::resolvePath(path);
  return resolved && QFileInfo(*resolved).isDir();
}

void FolderModel::openPath(const QString& path) {
  const auto resolved = DocumentSession::resolvePath(path);
  if (!resolved) {
    emit errorOccurred(resolved.error());
    return;
  }
  const QString root = QDir::cleanPath(*resolved);
  if (root == root_) {
    refresh();
    return;
  }
  start(root);
}

void FolderModel::refresh() {
  if (root_.isEmpty()) {
    return;
  }
  if (!scanning_.isEmpty()) {
    dirty_ = scanning_ == root_;  // A scan of another folder is about to replace this one anyway.
    return;
  }
  start(root_);
}

void FolderModel::start(const QString& root) {
  refresh_timer_.stop();
  if (cancel_) {
    cancel_->store(true);
  }
  cancel_ = std::make_shared<std::atomic_bool>(false);
  const quint64 generation = ++generation_;
  scanning_ = root;
  dirty_ = false;
  const QString loading = root == root_ ? QString() : root;
  if (loading != loading_) {
    loading_ = loading;
    emit loadingChanged();
  }
  auto* watcher = new QFutureWatcher<Scan>(this);
  connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher, generation, root] {
    Scan result = watcher->result();
    watcher->deleteLater();
    if (generation != generation_) {
      return;  // A newer scan superseded this one.
    }
    scanning_.clear();
    if (!loading_.isEmpty()) {
      loading_.clear();
      emit loadingChanged();
    }
    const bool rescan = dirty_;
    dirty_ = false;
    if (!result.error.isEmpty()) {
      emit errorOccurred(result.error);  // The previously adopted folder and tree stay.
    } else {
      adopt(root, std::move(result));
    }
    if (rescan && root == root_) {
      refresh_timer_.start();
    }
  });
  watcher->setFuture(QtConcurrent::run([root, cancel = cancel_] { return scan(root, cancel.get()); }));
}

void FolderModel::adopt(const QString& root, Scan result) {
  const bool new_root = root != root_;
  const bool changed = new_root || result.files != scan_.files || result.truncated != scan_.truncated;
  if (changed) {
    beginResetModel();
    if (new_root) {
      expanded_.clear();
    }
    root_ = root;
    scan_ = std::move(result);
    if (new_root) {
      reveal();
    }
    rebuildRows();
    endResetModel();
    ++revision_;
  }
  // A directory may have been replaced while retaining exactly the same listing.
  const QStringList watched = watcher_.directories();
  if (!watched.isEmpty()) {
    watcher_.removePaths(watched);
  }
  const QStringList wanted = scan_.directories.mid(0, kMaxWatches);
  if (!wanted.isEmpty()) {
    watcher_.addPaths(wanted);
  }
  if (new_root) {
    emit rootChanged();
  }
  if (changed) {
    emit treeChanged();
    updateCurrentRow();
  }
  if (new_root) {
    emit opened();
  }
}

// NOLINTNEXTLINE(misc-no-recursion)
void FolderModel::appendRows(const Node& parent, int depth, std::vector<Row>& rows) const {
  for (const Node& child : parent.children) {
    rows.push_back({.node = &child, .depth = depth});
    if (child.directory && expanded_.contains(child.path)) {
      appendRows(child, depth + 1, rows);
    }
  }
}

void FolderModel::rebuildRows() {
  rows_.clear();
  appendRows(scan_.tree, 0, rows_);
}

bool FolderModel::contains(const QUrl& url) const {
  return !root_.isEmpty() && url.isLocalFile() &&
         url.toLocalFile().startsWith(root_.endsWith(QLatin1Char('/')) ? root_ : root_ + QLatin1Char('/'));
}

// Expands the ancestors of the current document; returns whether expansion state changed.
bool FolderModel::reveal() {
  if (!contains(current_)) {
    return false;
  }
  bool changed = false;
  QString directory = QFileInfo(current_.toLocalFile()).absolutePath();
  while (directory.size() > root_.size()) {
    if (!expanded_.contains(directory)) {
      expanded_.insert(directory);
      changed = true;
    }
    directory = QFileInfo(directory).absolutePath();
  }
  return changed;
}

void FolderModel::setCurrent(const QUrl& url) {
  if (url == current_) {
    return;
  }
  current_ = url;
  emit currentChanged();
  if (reveal()) {
    beginResetModel();
    rebuildRows();
    endResetModel();
  }
  updateCurrentRow();
}

void FolderModel::updateCurrentRow() {
  int row = -1;
  if (current_.isLocalFile()) {
    const QString path = current_.toLocalFile();
    const auto found = std::ranges::find_if(
        rows_, [&path](const Row& candidate) { return !candidate.node->directory && candidate.node->path == path; });
    row = found == rows_.end() ? -1 : static_cast<int>(found - rows_.begin());
  }
  if (row != current_row_) {
    current_row_ = row;
    emit currentRowChanged();
  }
}

void FolderModel::toggle(int row) {
  if (std::cmp_greater_equal(row, rows_.size()) || !rows_[row].node->directory) {
    return;
  }
  const Node& node = *rows_[row].node;
  const int depth = rows_[row].depth;
  if (expanded_.remove(node.path)) {
    int last = row;
    while (last + 1 < static_cast<int>(rows_.size()) && rows_[last + 1].depth > depth) {
      ++last;
    }
    if (last > row) {
      beginRemoveRows({}, row + 1, last);
      rows_.erase(rows_.begin() + row + 1, rows_.begin() + last + 1);
      endRemoveRows();
    }
  } else {
    expanded_.insert(node.path);
    std::vector<Row> children;
    appendRows(node, depth + 1, children);
    if (!children.empty()) {
      beginInsertRows({}, row + 1, row + static_cast<int>(children.size()));
      rows_.insert(rows_.begin() + row + 1, children.begin(), children.end());
      endInsertRows();
    }
  }
  emit dataChanged(index(row), index(row), {static_cast<int>(Role::ExpandedRole)});
  updateCurrentRow();
}

int FolderModel::parentRow(int row) const {
  if (std::cmp_greater_equal(row, rows_.size())) {
    return -1;
  }
  const int depth = rows_[row].depth;
  for (int candidate = row - 1; candidate >= 0; --candidate) {
    if (rows_[candidate].depth < depth) {
      return candidate;
    }
  }
  return -1;
}

QVariantList FolderModel::search(const QString& query, int limit) const {
  const QString needle = query.trimmed();
  const QString base = root_.endsWith(QLatin1Char('/')) ? root_ : root_ + QLatin1Char('/');
  QVariantList by_name;
  QVariantList by_path;
  for (const QString& file : scan_.files) {
    if (by_name.size() >= limit) {
      break;
    }
    const qsizetype slash = file.lastIndexOf(QLatin1Char('/'));
    const QString file_name = file.mid(slash + 1);
    const bool name_match = needle.isEmpty() || file_name.contains(needle, Qt::CaseInsensitive);
    if (!name_match && (by_path.size() >= limit || !file.contains(needle, Qt::CaseInsensitive))) {
      continue;
    }
    (name_match ? by_name : by_path)
        .push_back(QVariantMap{{QStringLiteral("name"), file_name},
                               {QStringLiteral("directory"), slash < 0 ? QString() : file.left(slash)},
                               {QStringLiteral("url"), QUrl::fromLocalFile(base + file)}});
  }
  by_name.append(by_path);
  return by_name.mid(0, limit);
}
