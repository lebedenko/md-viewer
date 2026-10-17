#include <QCommandLineParser>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QTimer>

int main(int argc, char* argv[]) {
  const QGuiApplication app(argc, argv);
  QCoreApplication::setApplicationName(QStringLiteral("mdv"));
  QGuiApplication::setApplicationDisplayName(QStringLiteral("Markdown Viewer"));
  QGuiApplication::setDesktopFileName(QStringLiteral("mdv"));
  QGuiApplication::setWindowIcon(QIcon(QStringLiteral(":/assets/mdv.svg")));
  QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));
  QCommandLineParser parser;
  parser.setApplicationDescription(QStringLiteral("A local Markdown reader"));
  parser.addHelpOption();
  parser.addVersionOption();
  const QCommandLineOption smoke_option(QStringLiteral("smoke-test"), QStringLiteral("Check QML startup and exit."));
  parser.addOption(smoke_option);
  parser.addPositionalArgument(QStringLiteral("file"), QStringLiteral("Markdown file to open."),
                               QStringLiteral("[file]"));
  parser.process(app);
  const QStringList files = parser.positionalArguments();
  if (files.size() > 1) {
    qCritical("Expected at most one file, got %lld.", static_cast<long long>(files.size()));
    return 2;
  }

  QQmlApplicationEngine engine;
  engine.addImportPath(QString::fromUtf8(MD_VIEWER_QMARKDOWN_IMPORT_PATH));
  if (!files.isEmpty()) {
    engine.setInitialProperties({{QStringLiteral("initialPath"), files.constFirst()}});
  }
  engine.loadFromModule(QStringLiteral("MdViewer"), QStringLiteral("Main"));
  if (engine.rootObjects().isEmpty()) {
    return 1;
  }
  if (parser.isSet(smoke_option)) {
    QTimer::singleShot(0, &app, &QCoreApplication::quit);
  }
  return QGuiApplication::exec();
}
