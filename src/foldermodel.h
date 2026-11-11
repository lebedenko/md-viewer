#pragma once

#include <QAbstractListModel>
#include <QFileSystemWatcher>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <atomic>
#include <memory>
#include <vector>

// One opened folder as a flat list of visible tree rows. A recursive background scan keeps only
// Markdown files and the directories that lead to them; expansion state lives here, not in the view.
class FolderModel : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QString root READ root NOTIFY rootChanged)
  Q_PROPERTY(QString name READ name NOTIFY rootChanged)
  Q_PROPERTY(QString displayPath READ displayPath NOTIFY rootChanged)
  Q_PROPERTY(QString loading READ loading NOTIFY loadingChanged)
  Q_PROPERTY(bool truncated READ truncated NOTIFY treeChanged)
  Q_PROPERTY(int fileCount READ fileCount NOTIFY treeChanged)
  Q_PROPERTY(int revision READ revision NOTIFY treeChanged)
  Q_PROPERTY(QUrl readme READ readme NOTIFY treeChanged)
  Q_PROPERTY(QUrl current READ current WRITE setCurrent NOTIFY currentChanged)
  Q_PROPERTY(int currentRow READ currentRow NOTIFY currentRowChanged)

 public:
  enum class Role : quint16 { NameRole = Qt::UserRole + 1, UrlRole, DirectoryRole, DepthRole, ExpandedRole };

  static constexpr int kMaxFiles = 20'000;
  static constexpr int kMaxDirectories = 200'000;
  static constexpr int kMaxWatches = 1024;
  static constexpr int kRefreshDebounceMs = 300;

  struct Node {
    QString name;
    QString path;  // Absolute and lexical: symlinks are followed, never resolved into the shown path.
    bool directory = false;
    std::vector<Node> children;
  };
  struct Scan {
    Node tree;
    QStringList files;        // Root-relative, in tree order.
    QStringList directories;  // Absolute listed directories, root first.
    bool truncated = false;
    QString error;
  };

  explicit FolderModel(QObject* parent = nullptr);
  ~FolderModel() override;
  FolderModel(const FolderModel&) = delete;
  FolderModel& operator=(const FolderModel&) = delete;
  FolderModel(FolderModel&&) = delete;
  FolderModel& operator=(FolderModel&&) = delete;

  // Synchronous and self-contained so it can run on a worker thread; `cancel` stops it early.
  static Scan scan(const QString& root, const std::atomic_bool* cancel = nullptr, int maxFiles = kMaxFiles);
  static bool isMarkdown(const QString& fileName);

  [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
  [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

  [[nodiscard]] QString root() const { return root_; }
  [[nodiscard]] QString name() const;
  [[nodiscard]] QString displayPath() const;
  [[nodiscard]] QString loading() const { return loading_; }
  [[nodiscard]] bool truncated() const { return scan_.truncated; }
  [[nodiscard]] int fileCount() const { return static_cast<int>(scan_.files.size()); }
  [[nodiscard]] int revision() const { return revision_; }
  [[nodiscard]] QUrl readme() const;
  [[nodiscard]] QUrl current() const { return current_; }
  [[nodiscard]] int currentRow() const { return current_row_; }
  void setCurrent(const QUrl& url);

  // Accepts the same spellings as DocumentSession::resolvePath (relative, ~, file: URLs).
  Q_INVOKABLE static bool isDirectory(const QString& path);
  Q_INVOKABLE void openPath(const QString& path);
  Q_INVOKABLE void refresh();
  Q_INVOKABLE [[nodiscard]] bool contains(const QUrl& url) const;
  Q_INVOKABLE void toggle(int row);
  Q_INVOKABLE [[nodiscard]] int parentRow(int row) const;
  // Case-insensitive substring match on the root-relative path; file-name matches rank first.
  Q_INVOKABLE [[nodiscard]] QVariantList search(const QString& query, int limit = 50) const;

 signals:
  void rootChanged();
  void loadingChanged();
  void treeChanged();
  void currentChanged();
  void currentRowChanged();
  void opened();  // The first scan of a newly opened folder was adopted.
  void errorOccurred(const QString& message);

 private:
  struct Row {
    const Node* node = nullptr;
    int depth = 0;
  };
  void start(const QString& root);
  void adopt(const QString& root, Scan result);
  void appendRows(const Node& parent, int depth, std::vector<Row>& rows) const;
  void rebuildRows();
  void updateCurrentRow();
  bool reveal();

  QString root_;
  QString loading_;
  Scan scan_;
  std::vector<Row> rows_;
  QSet<QString> expanded_;
  QUrl current_;
  int current_row_ = -1;
  int revision_ = 0;
  QString scanning_;  // Root of the scan in flight, if any.
  bool dirty_ = false;
  quint64 generation_ = 0;
  std::shared_ptr<std::atomic_bool> cancel_;
  QFileSystemWatcher watcher_;
  QTimer refresh_timer_;
};
