//
// $Id$

#ifndef IMPORT_EXPORT_MANAGER
#define IMPORT_EXPORT_MANAGER

#include "http/HttpManager.h"

class QUrl;

class Callback;

/**
 * Handles imports and exports.
 */
class ImportExportManager : public QObject, public HttpRequestHandler
{
    Q_OBJECT

public:

    /**
     * Creates the import/export manager.
     */
    ImportExportManager (ServerApp* app);

    /**
     * Makes an import page available for submitting an import.
     *
     * @return the URL from which the user may submit the import.
     */
    QUrl addImport (const QString& name, const Callback& callback);

    /**
     * Makes an export available for downloading.
     *
     * @return the URL from which the export may be retrieved.
     */
    QUrl addExport (const QString& name, const QByteArray& content);

    /**
     * Handles an HTTP request.
     */
    virtual void handleRequest (
        HttpConnection* connection, const QString& name, const QString& path);

protected:

    /** The server application. */
    ServerApp* _app;
};

#endif // IMPORT_EXPORT_MANAGER
