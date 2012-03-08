//
// $Id$

#include <string.h>

#include <QMetaType>

#include "util/General.h"

int qIntVectorType = qRegisterMetaType<QIntVector>("QIntVector");

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
