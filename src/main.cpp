#include <QCommandLineParser>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QTimer>

int main(int argc, char* argv[]) {
  const QGuiApplication app(argc, argv);
  QCoreApplication::setApplicationName(QStringLiteral("md-viewer"));
  QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));
  QCommandLineParser parser;
  parser.setApplicationDescription(QStringLiteral("A local Markdown reader"));
  parser.addHelpOption();
  parser.addVersionOption();
  const QCommandLineOption smoke_option(QStringLiteral("smoke-test"), QStringLiteral("Check QML startup and exit."));
  parser.addOption(smoke_option);
  parser.process(app);

  QQmlApplicationEngine engine;
  engine.addImportPath(QString::fromUtf8(MD_VIEWER_QMARKDOWN_IMPORT_PATH));
  engine.loadFromModule(QStringLiteral("MdViewer"), QStringLiteral("Main"));
  if (engine.rootObjects().isEmpty()) {
    return 1;
  }
  if (parser.isSet(smoke_option)) {
    QTimer::singleShot(0, &app, &QCoreApplication::quit);
  }
  return QGuiApplication::exec();
}
