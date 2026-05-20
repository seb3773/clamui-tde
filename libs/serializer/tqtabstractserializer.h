#ifndef TQTABSTRACTSERIALIZER_H
#define TQTABSTRACTSERIALIZER_H

#include <ntqstring.h>
#include <ntqvariant.h>

class TQtAbstractSerializer {
public:
    TQtAbstractSerializer() {}
    virtual ~TQtAbstractSerializer() {}

    TQVariant deserialize(const TQString& value, const TQVariant::Type type) const;
    TQString serialize(const TQVariant& value) const;

protected:
    virtual TQString escapeString(const TQString& str) const;
    virtual TQString unescapeString(const TQString& str) const;

    virtual TQVariant fromString(const TQString& value, const TQVariant::Type type) const = 0;
    virtual TQString toString(const TQVariant& value) const = 0;
};

#endif
