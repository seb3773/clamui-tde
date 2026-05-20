#ifndef TQTBINARYSERIALIZER_H
#define TQTBINARYSERIALIZER_H

#include "tqtabstractserializer.h"

class TQtBinarySerializer : public TQtAbstractSerializer {
public:
    TQtBinarySerializer() {}
    virtual ~TQtBinarySerializer() {}

protected:
    virtual TQVariant fromString(const TQString& value, const TQVariant::Type type) const;
    virtual TQString toString(const TQVariant& value) const;
};

#endif
