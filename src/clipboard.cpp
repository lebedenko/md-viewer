#include "clipboard.h"

#include <QClipboard>
#include <QGuiApplication>

Clipboard::Clipboard(QObject* parent) : QObject(parent) {}

void Clipboard::copyText(const QString& text) { QGuiApplication::clipboard()->setText(text); }
