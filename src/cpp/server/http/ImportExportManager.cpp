//
// $Id$

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

QUrl ImportExportManager::addImport (const QString& name, const Callback& callback)
{
    return QUrl();
}

QUrl ImportExportManager::addExport (const QString& name, const QByteArray& content)
{
    return QUrl();
}

void ImportExportManager::handleRequest (
    HttpConnection* connection, const QString& name, const QString& path)
{
    connection->respond("200 OK", name.toAscii());
}
