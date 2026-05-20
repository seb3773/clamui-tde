#ifndef TQTSTRINGSERIALIZER_H
#define TQTSTRINGSERIALIZER_H

#include "tqtabstractserializer.h"

#include <ntqstringlist.h>
#include <ntqdatetime.h>
#include <ntqpoint.h>
#include <ntqrect.h>
#include <ntqsize.h>
#include <ntquuid.h>

class TQtStringSerializer : public TQtAbstractSerializer {
public:
    TQtStringSerializer() {}
    virtual ~TQtStringSerializer() {}

    bool readString(TQString& text, TQString& out) const;

protected:
    virtual TQVariant fromString(const TQString& value, const TQVariant::Type type) const;
    virtual TQString toString(const TQVariant& value) const;

    virtual TQString escapeString(const TQString& str) const;
    virtual TQString unescapeString(const TQString& str) const;

private:
    TQString fromVariantList(const TQValueList<TQVariant>& list) const;
};

#endif
