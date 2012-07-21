//
// $Id$

#ifndef IMPORT_EXPORT_MANAGER
#define IMPORT_EXPORT_MANAGER

#include "http/HttpManager.h"
#include "util/Callback.h"

class QUrl;

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
     * Makes an import page available for submitting an import.  The callback will receive the
     * QUrl from which the user may submit the import.
     *
     * @param receiver the receiver that will receive the QByteArray content after upload.
     */
    Q_INVOKABLE void addImport (
        const QString& name, const Callback& receiver, const Callback& callback);

    /**
     * Makes an export available for downloading.  The callback will receive the QUrl from which
     * the export may be retrieved.
     */
    Q_INVOKABLE void addExport (
        const QString& name, const QByteArray& content, const Callback& callback);

    /**
     * Handles an HTTP request.
     */
    virtual bool handleRequest (
        HttpConnection* connection, const QString& name, const QString& path);

protected slots:

    /**
     * Cancels a pending import.
     */
    void cancelImport ();

    /**
     * Cancels a pending export.
     */
    void cancelExport ();

protected:

    /** The server application. */
    ServerApp* _app;

    /** Pending imports mapped by token. */
    QHash<QString, Callback> _imports;

    /** Pending exports mapped by token. */
    QHash<QString, QByteArray> _exports;
};

#endif // IMPORT_EXPORT_MANAGER
