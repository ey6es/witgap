//
// $Id$

#include <QTimer>
#include <QtDebug>

#include "ServerApp.h"
#include "http/HttpConnection.h"
#include "http/ImportExportManager.h"

ImportExportManager::ImportExportManager (ServerApp* app) :
    QObject(app),
    _app(app)
{
    // register for /import and /export
    _app->httpManager()->registerSubhandler("import", this);
    _app->httpManager()->registerSubhandler("export", this);
}

/** The timeout for pending imports and exports. */
const int ImportExportTimeout = 60 * 1000;

void ImportExportManager::addImport (
    const QString& name, const Callback& receiver, const Callback& callback)
{
    QString token;
    do {
        token = generateToken(16).toHex();
    } while (_imports.contains(token));

    _imports.insert(token, receiver);
    callback.invoke(Q_ARG(const QUrl&, QUrl(
        _app->httpManager()->baseUrl() + "/import/" + name + "?token=" + token)));

    QTimer* timer = new QTimer(this);
    timer->setProperty("token", token);
    connect(timer, SIGNAL(timeout()), SLOT(cancelImport()));
    timer->connect(timer, SIGNAL(timeout()), SLOT(deleteLater()));
    timer->start(ImportExportTimeout);
}

void ImportExportManager::addExport (
    const QString& name, const QByteArray& content, const Callback& callback)
{
    QString token;
    do {
        token = generateToken(16).toHex();
    } while(_exports.contains(token));

    _exports.insert(token, content);
    callback.invoke(Q_ARG(const QUrl&, QUrl(
        _app->httpManager()->baseUrl() + "/export/" + name + "?token=" + token)));

    QTimer* timer = new QTimer(this);
    timer->setProperty("token", token);
    connect(timer, SIGNAL(timeout()), SLOT(cancelExport()));
    timer->connect(timer, SIGNAL(timeout()), SLOT(deleteLater()));
    timer->start(ImportExportTimeout);
}

bool ImportExportManager::handleRequest (
    HttpConnection* connection, const QString& name, const QString& path)
{
    QString token = connection->requestUrl().queryItemValue("token");
    if (name == "export") {
        QByteArray content = _exports.value(token);
        if (!content.isNull()) {
            connection->respond("200 OK", content, "application/octet-stream");
            return true;
        }
    }
    if (!_imports.contains(token)) {
        return false;
    }
    if (connection->requestOperation() != QNetworkAccessManager::PostOperation) {
        connection->respond("200 OK",
            "<html><body><form action=\"" + path.toAscii() + "?token=" + token.toAscii() +
                     + "\" enctype=\"multipart/form-data\" method=\"post\">"
                "<input type=\"file\" name=\"content\"/>"
                "<input type=\"submit\" value=\"Submit\"/>"
            "</form></body></html>",
            "text/html; charset=ISO-8859-1");
        return true;
    }
    QList<FormData> data = connection->parseFormData();
    if (data.isEmpty()) {
        connection->respond("400 Bad Request", "Missing form data.");
        return true;
    }
    Callback& ref = _imports[token];
    ref.invoke(Q_ARG(const QByteArray&, data.at(0).second));
    ref = Callback();
    connection->respond("200 OK", "Submitted.");
    return true;
}

void ImportExportManager::cancelImport ()
{
    _imports.remove(sender()->property("token").toString());
}

void ImportExportManager::cancelExport ()
{
    _exports.remove(sender()->property("token").toString());
}
