//
// $Id$

#include <iostream>

#include <signal.h>

#include <QByteArray>
#include <QDateTime>
#include <QMetaObject>
#include <QStringList>
#include <QThreadPool>
#include <QTimer>
#include <QTranslator>
#include <QtDebug>

#include "ServerApp.h"
#include "db/DatabaseThread.h"
#include "net/ConnectionManager.h"
#include "scene/SceneManager.h"
#include "util/General.h"
#include "util/Mailer.h"

using namespace std;

// anything we "translate" needs to be an untranslated key
#define tr(...) TranslationKey("ServerApp", __VA_ARGS__)

volatile int pendingSignal = 0;

/**
 * Signal handler that simply sets a variable that the main loop checks periodically.
 */
void signalHandler (int signal)
{
    pendingSignal = signal;
}

/**
 * Returns a string to use in the log entry for a message of the given type.
 */
const char* typeString (QtMsgType type)
{
    switch (type) {
        case QtDebugMsg: return " INFO: ";
        case QtWarningMsg: return " WARNING: ";
        case QtCriticalMsg: return " CRITICAL: ";
        case QtFatalMsg: return " FATAL: ";
        default: return " ???: ";
    }
}

/**
 * Debug message handler that prepends the log time and sends the messages to the main event
 * loop (for thread safety).
 */
void logHandler (QtMsgType type, const char* msg)
{
    static QMetaMethod method = ServerApp::staticMetaObject.method(
        ServerApp::staticMetaObject.indexOfMethod("log(QByteArray)"));
    QByteArray timestamp = QDateTime::currentDateTime().toString(Qt::ISODate).toAscii();
    QCoreApplication* app = QCoreApplication::instance();
    if (app == 0 || app->thread() == QThread::currentThread()) {
        cout << timestamp.constData() << typeString(type) << msg << endl;
    } else {
        method.invoke(app, Q_ARG(const QByteArray&, timestamp + typeString(type) + msg));
    }
}

/**
 * A translator that returns the untranslated text if a translation isn't found.
 */
class SourceTranslator : public QTranslator
{
public:

    /**
     * Creates a new source translator.
     */
    SourceTranslator (QObject* parent = 0) : QTranslator(parent) { }

    /**
     * Translates a string.
     */
    virtual QString translate (
        const char* context, const char* sourceText, const char* disambiguation = 0) const;
};

QString SourceTranslator::translate (
    const char* context, const char* sourceText, const char* disambiguation) const
{
    QString result = QTranslator::translate(context, sourceText, disambiguation);
    return result.isEmpty() ? sourceText : result;
}

/**
 * A translator that forwards requests without translations to a parent.
 */
class ForwardingTranslator : public QTranslator
{
public:

    /**
     * Creates a new forwarding translator.
     */
    ForwardingTranslator (QTranslator* parent) : QTranslator(parent), _parent(parent) { }

    /**
     * Translates a string.
     */
    virtual QString translate (
        const char* context, const char* sourceText, const char* disambiguation = 0) const;

protected:

    /** The parent translator. */
    QTranslator* _parent;
};

QString ForwardingTranslator::translate (
    const char* context, const char* sourceText, const char* disambiguation) const
{
    QString result = QTranslator::translate(context, sourceText, disambiguation);
    return result.isEmpty() ? _parent->translate(context, sourceText, disambiguation) : result;
}

ServerApp::ServerApp (int& argc, char** argv, const QString& configFile) :
    QCoreApplication(argc, argv),
    _config(configFile, QSettings::IniFormat, this),
    _clientUrl(_config.value("client_url").toString()),
    _bugReportAddress(_config.value("bug_report_address").toString()),
    _mailHostname(_config.value("mail_hostname").toString()),
    _mailPort(_config.value("mail_port").toUInt()),
    _mailFrom(_config.value("mail_from").toString())
{
    // register the signal handler for ^C/HUP
    signal(SIGINT, signalHandler);
    signal(SIGHUP, signalHandler);

    // register the log handler
    qInstallMsgHandler(logHandler);

    // load translators for supported locales
    QString tdir = _config.value("translation_directory").toString();
    foreach (const QString& locale, _config.value("translation_locales").toString().split(' ')) {
        QTranslator* translator;
        if (locale == "en") {
            translator = new SourceTranslator(this);
        } else {
            // forward to the parent language, if any; else, to English
            int idx = locale.indexOf('_');
            translator = new ForwardingTranslator(_translators.value(
                idx == -1 ? "en" : locale.left(idx)));
        }
        if (!translator->load(locale, tdir)) {
            qWarning() << "Failed to load translation file." << locale;
        }
        _translators.insert(locale, translator);
    }

    // start the idle timer
    QTimer* timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), SLOT(idle()));
    timer->start(100);

    // create and connect the reboot timer
    _rebootTimer = new QTimer(this);
    connect(_rebootTimer, SIGNAL(timeout()), SLOT(updateReboot()));

    // create the database thread
    _databaseThread = new DatabaseThread(this);

    // create the managers
    _connectionManager = new ConnectionManager(this);
    _sceneManager = new SceneManager(this);

    // start the database and scene threads
    _databaseThread->start();
    _sceneManager->startThreads();

    // connect the cleanup signal
    connect(this, SIGNAL(aboutToQuit()), SLOT(cleanup()));

    // note startup
    qDebug() << "Server initialized.";
}

QString ServerApp::translationLocale (const QString& locale) const
{
    // first look for an exact match
    if (_translators.contains(locale)) {
        return locale;
    }
    // if there's a separator, look for just the language
    int idx = locale.indexOf('-');
    if (idx == -1) {
        idx = locale.indexOf('_');
    }
    if (idx != -1) {
        QString language = locale.left(idx);
        if (_translators.contains(language)) {
            return language;
        }
    }
    // return the default
    return "en";
}

void ServerApp::sendMail (const QString& to, const QString& subject,
    const QString& body, const Callback& callback)
{
    QThreadPool::globalInstance()->start(new Mailer(
        _mailHostname, _mailPort, _mailFrom, to, subject, body, callback));
}

/** The times in minutes of the stages of the reboot countdown. */
static const int RebootCountdownMinutes[] = { 30, 15, 10, 5, 2, 0 };

/** The number of stages in the reboot countdown. */
static const int RebootCountdownLength = sizeof(RebootCountdownMinutes) / sizeof(int);

void ServerApp::scheduleReboot (int minutes, const QString& message)
{
    for (int ii = 0; ii < RebootCountdownLength; ii++) {
        if (minutes >= RebootCountdownMinutes[ii]) {
            _rebootCountdownIdx = ii;
            _rebootTimer->start((minutes - RebootCountdownMinutes[ii]) * 60 * 1000);
            break;
        }
    }
    _rebootMessage = message.isEmpty() ? message : ("  " + message);
}

void ServerApp::cancelReboot ()
{
    _rebootTimer->stop();
}

void ServerApp::idle ()
{
    if (pendingSignal == SIGINT) {
        exit();

    } else if (pendingSignal == SIGQUIT) {
        // TODO: some kind of debug output

        pendingSignal = 0;
    }
}

void ServerApp::updateReboot ()
{
    if (_rebootCountdownIdx == RebootCountdownLength) {
        exit();
        return;
    }
    int minutes = RebootCountdownMinutes[_rebootCountdownIdx++];
    if (minutes == 0) {
        _connectionManager->broadcast(tr("The server will now reboot!"));
        _rebootTimer->start(2 * 1000);
    } else {
        _connectionManager->broadcast(
            tr("The server will be rebooted in %1 minutes.%2").arg(
                QString::number(minutes), _rebootMessage));
        _rebootTimer->start((minutes - RebootCountdownMinutes[_rebootCountdownIdx]) * 60 * 1000);
    }
}

void ServerApp::cleanup ()
{
    // shut down the scene threads
    _sceneManager->stopThreads();

    // shut down the database thread
    _databaseThread->exit();
    _databaseThread->wait();

    // process events again in case debug messages were enqueued
    processEvents();
}

void ServerApp::log (const QByteArray& msg)
{
    cout << msg.constData() << endl;
}
