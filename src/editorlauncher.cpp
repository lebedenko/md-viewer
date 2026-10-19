#include "editorlauncher.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QProcess>

namespace {
constexpr auto kFilePlaceholder = QLatin1StringView("{file}");
}  // namespace

std::expected<EditorLauncher::Invocation, QString> EditorLauncher::buildInvocation(const QStringList& command,
                                                                                   const QString& file) {
  if (command.isEmpty() || command.constFirst().isEmpty()) {
    return std::unexpected(tr("No editor command configured"));
  }
  Invocation invocation{.program = command.constFirst(), .arguments = command.sliced(1)};
  bool substituted = false;
  for (QString& argument : invocation.arguments) {
    if (argument.contains(kFilePlaceholder)) {
      argument.replace(kFilePlaceholder, file);
      substituted = true;
    }
  }
  if (!substituted) {
    invocation.arguments.append(file);
  }
  return invocation;
}

EditorLauncher::EditorLauncher(QObject* parent) : QObject(parent) {}

void EditorLauncher::setCommand(const QStringList& command) {
  if (command != command_) {
    command_ = command;
    emit commandChanged();
  }
}

bool EditorLauncher::openInEditor(const QUrl& file) {
  if (!file.isLocalFile()) {
    setError(tr("Only local files can be opened in an editor"));
    return false;
  }
  const QString path = file.toLocalFile();
  if (command_.isEmpty()) {
    if (!QDesktopServices::openUrl(file)) {
      setError(tr("Cannot open %1 with the desktop default application").arg(path));
      return false;
    }
    setError({});
    return true;
  }
  const auto invocation = buildInvocation(command_, path);
  if (!invocation) {
    setError(invocation.error());
    return false;
  }
  QProcess process;
  process.setProgram(invocation->program);
  process.setArguments(invocation->arguments);
  process.setWorkingDirectory(QFileInfo(path).absolutePath());
  if (!process.startDetached()) {
    setError(tr("Cannot start editor '%1': %2").arg(invocation->program, process.errorString()));
    return false;
  }
  setError({});
  return true;
}

void EditorLauncher::setError(const QString& error) {
  if (!error.isEmpty()) {
    emit errorOccurred(error);
  }
  if (error != error_) {
    error_ = error;
    emit errorChanged();
  }
}
