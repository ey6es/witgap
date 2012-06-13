//
// $Id$

#ifndef STREAMING
#define STREAMING

#include <QMetaType>

/** Flags a class as streamable (use as you would Q_OBJECT). */
#define STREAMABLE public: static const int Type; private:

/** Flags a field or base class as streaming. */
#define STREAM

/** Declares the metatype and the streaming operators.  The last line
 * ensures that the generated file will be included in the link phase. */
#define DECLARE_STREAMABLE_METATYPE(X) Q_DECLARE_METATYPE(X) \
    QDataStream& operator<< (QDataStream& out, const X& obj); \
    QDataStream& operator>> (QDataStream& in, X& obj); \
    static const int* _TypePtr##X = &X::Type;

/** Declares the usual and streaming operators for a flag type. */
#define DECLARE_STREAMING_OPERATORS_FOR_FLAGS(X) Q_DECLARE_OPERATORS_FOR_FLAGS(X) \
    inline QDataStream& operator<< (QDataStream& out, const X& flags) { out << (quint32)flags; } \
    inline QDataStream& operator>> (QDataStream& in, X& flags) \
    { quint32 v; in >> v; flags = (QFlag)v; }

/**
 * Registers a streamable type and its streaming operators.
 */
template<class T> int registerStreamableType (const char* typeName)
{
    int type = qRegisterMetaType<T>();
    qRegisterMetaTypeStreamOperators<T>(typeName);
    return type;
}

#endif // STREAMING
