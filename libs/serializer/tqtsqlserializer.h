#ifndef TQTSQLSERIALIZER_H
#define TQTSQLSERIALIZER_H

#include "tqtstringserializer.h"

class TQtSqlSerializer : public TQtStringSerializer {
public:
    TQtSqlSerializer() {}
    virtual ~TQtSqlSerializer() {}

protected:
    virtual TQString escapeString(const TQString& str) const;
    virtual TQString unescapeString(const TQString& str) const;
};

#endif
