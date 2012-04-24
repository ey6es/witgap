//
// $Id$

#include <string.h>

#include <QDateTime>
#include <QMetaType>
#include <QString>
#include <QVariant>

#include "Protocol.h"
#include "util/General.h"

// register our types with the metatype system
int qIntVectorType = qRegisterMetaType<QIntVector>("QIntVector");
int qVariantType = qRegisterMetaType<QVariant>("QVariant");

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
