//
// $Id$

#include <string.h>

#include <QDateTime>
#include <QIODevice>
#include <QMetaType>
#include <QString>
#include <QTranslator>
#include <QVariant>
#include <QtDebug>

#include "Protocol.h"
#include "util/General.h"

// register our types with the metatype system
int qIntVectorType = qRegisterMetaType<QIntVector>("QIntVector");
int translationKeyType = qRegisterMetaType<TranslationKey>("TranslationKey");
int qVariantType = qRegisterMetaType<QVariant>("QVariant");

Creator::Creator (QObject* parent, const QMetaObject* metaObject,
    QGenericArgument val0, QGenericArgument val1, QGenericArgument val2, QGenericArgument val3,
    QGenericArgument val4, QGenericArgument val5, QGenericArgument val6, QGenericArgument val7,
    QGenericArgument val8, QGenericArgument val9) :
        QObject(parent),
        _metaObject(metaObject)
{
    QGenericArgument args[] = { val0, val1, val2, val3, val4, val5, val6, val7, val8, val9 };
    for (int ii = 0; ii < 10 && args[ii].name() != 0; ii++) {
        int type = QMetaType::type(args[ii].name());
        _args[ii].first = type;
        _args[ii].second = QMetaType::construct(type, args[ii].data());
    }
}

Creator::~Creator ()
{
    // destroy the copies we created
    for (int ii = 0; ii < 10 && _args[ii].first != 0; ii++) {
        QMetaType::destroy(_args[ii].first, _args[ii].second);
    }
}

QObject* Creator::create ()
{
    QGenericArgument args[10];
    for (int ii = 0; ii < 10 && _args[ii].first != 0; ii++) {
        args[ii] = QGenericArgument(QMetaType::typeName(_args[ii].first), _args[ii].second);
    }
    return _metaObject->newInstance(args[0], args[1], args[2], args[3], args[4],
        args[5], args[6], args[7], args[8], args[9]);
}

QIntVector QIntVector::createHighlighted (const QString& string)
{
    QIntVector vector;
    for (const QChar* ptr = string.constData(), *end = ptr + string.length(); ptr < end; ptr++) {
        QChar ch = *ptr;
        if (ch == '&' && ++ptr < end && *ptr != '&') {
            vector.append((*ptr).unicode() | REVERSE_FLAG);
        } else {
            vector.append(ch.unicode());
        }
    }
    return vector;
}

QIntVector::QIntVector ()
{
}

QIntVector::QIntVector (int size, int value) :
    QVector<int>(size, value)
{
}

QIntVector::QIntVector (const char* string, int style) :
    QVector<int>(strlen(string))
{
    int* dptr = data();
    for (const char* sptr = string, *end = sptr + size(); sptr < end; sptr++) {
        *dptr++ = (*sptr) | style;
    }
}

QIntVector::QIntVector (const QString& string, int style) :
    QVector<int>(string.length())
{
    int* dptr = data();
    for (const QChar* sptr = string.constData(), *end = sptr + string.length();
            sptr < end; sptr++) {
        *dptr++ = (*sptr).unicode() | style;
    }
}

TranslationKey::TranslationKey (
        const char* context, const char* sourceText, const char* disambiguation, int n) :
    _context(context),
    _sourceText(sourceText),
    _disambiguation(disambiguation),
    _n(n)
{
}

TranslationKey::TranslationKey ()
{
}

TranslationKey& TranslationKey::arg (const QString& a1)
{
    _args.append(a1);
    return *this;
}

TranslationKey& TranslationKey::arg (const QString& a1, const QString& a2)
{
    _args.append(a1);
    _args.append(a2);
    return *this;
}

TranslationKey& TranslationKey::arg (const QString& a1, const QString& a2, const QString& a3)
{
    _args.append(a1);
    _args.append(a2);
    _args.append(a3);
    return *this;
}

TranslationKey& TranslationKey::arg (
    const QString& a1, const QString& a2, const QString& a3, const QString& a4)
{
    _args.append(a1);
    _args.append(a2);
    _args.append(a3);
    _args.append(a4);
    return *this;
}

TranslationKey& TranslationKey::arg (
    const QString& a1, const QString& a2, const QString& a3, const QString& a4, const QString& a5)
{
    _args.append(a1);
    _args.append(a2);
    _args.append(a3);
    _args.append(a4);
    _args.append(a5);
    return *this;
}

TranslationKey& TranslationKey::arg (
    const QString& a1, const QString& a2, const QString& a3, const QString& a4, const QString& a5,
    const QString& a6)
{
    _args.append(a1);
    _args.append(a2);
    _args.append(a3);
    _args.append(a4);
    _args.append(a5);
    _args.append(a6);
    return *this;
}

TranslationKey& TranslationKey::arg (
    const QString& a1, const QString& a2, const QString& a3, const QString& a4, const QString& a5,
    const QString& a6, const QString& a7)
{
    _args.append(a1);
    _args.append(a2);
    _args.append(a3);
    _args.append(a4);
    _args.append(a5);
    _args.append(a6);
    _args.append(a7);
    return *this;
}

TranslationKey& TranslationKey::arg (
    const QString& a1, const QString& a2, const QString& a3, const QString& a4, const QString& a5,
    const QString& a6, const QString& a7, const QString& a8)
{
    _args.append(a1);
    _args.append(a2);
    _args.append(a3);
    _args.append(a4);
    _args.append(a5);
    _args.append(a6);
    _args.append(a7);
    _args.append(a8);
    return *this;
}

TranslationKey& TranslationKey::arg (
    const QString& a1, const QString& a2, const QString& a3, const QString& a4, const QString& a5,
    const QString& a6, const QString& a7, const QString& a8, const QString& a9)
{
    _args.append(a1);
    _args.append(a2);
    _args.append(a3);
    _args.append(a4);
    _args.append(a5);
    _args.append(a6);
    _args.append(a7);
    _args.append(a8);
    _args.append(a9);
    return *this;
}

QString TranslationKey::translate (const QTranslator* translator) const
{
    QString base = translator->translate(_context, _sourceText, _disambiguation, _n);
    switch (_args.size()) {
        case 0:
            return base;

        case 1:
            return base.arg(_args[0]);

        case 2:
            return base.arg(_args[0], _args[1]);

        case 3:
            return base.arg(_args[0], _args[1], _args[2]);

        case 4:
            return base.arg(_args[0], _args[1], _args[2], _args[3]);

        case 5:
            return base.arg(_args[0], _args[1], _args[2], _args[3], _args[4]);

        case 6:
            return base.arg(_args[0], _args[1], _args[2], _args[3], _args[4], _args[5]);

        case 7:
            return base.arg(_args[0], _args[1], _args[2], _args[3], _args[4], _args[5], _args[6]);

        case 8:
            return base.arg(_args[0], _args[1], _args[2], _args[3], _args[4], _args[5], _args[6],
                _args[7]);

        default:
        case 9:
            return base.arg(_args[0], _args[1], _args[2], _args[3], _args[4], _args[5], _args[6],
                _args[7], _args[8]);
    }
}

Throttle::Throttle (int count, quint64 interval) :
    _buckets(count),
    _interval(interval),
    _bucketIdx(0)
{
}

bool Throttle::attemptOp ()
{
    quint64 now = currentTimeMillis();
    if (now - _buckets.at(_bucketIdx) < _interval) {
        return false;
    }
    _buckets[_bucketIdx] = now;
    _bucketIdx = (_bucketIdx + 1) % _buckets.size();
    return true;
}

QByteArray generateToken (int length)
{
    QByteArray token(length, 0);
    for (char* ptr = token.data(), *end = ptr + length; ptr < end; ptr++) {
        *ptr = qrand() % 256;
    }
    return token;
}

quint64 currentTimeMillis ()
{
    QDateTime now = QDateTime::currentDateTime();
    return (quint64)now.toTime_t()*1000 + now.time().msec();
}

int randomIndex (const double* probs)
{
    double total = qrand() / (double)RAND_MAX;
    for (int ii = 0;; ii++) {
        if ((total -= probs[ii]) <= 0.0) {
            return ii;
        }
    }
}

void unget (QIODevice* device, quint32 value)
{
    device->ungetChar(value & 0xFF);
    device->ungetChar((value >> 8) & 0xFF);
    device->ungetChar((value >> 16) & 0xFF);
    device->ungetChar(value >> 24);
}
