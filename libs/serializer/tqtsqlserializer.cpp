#include "tqtsqlserializer.h"

TQString TQtSqlSerializer::escapeString(const TQString& str) const {
    TQString ret(str);
    return ret.replace("'", "''");
}

TQString TQtSqlSerializer::unescapeString(const TQString& str) const {
    TQString ret(str);
    return ret.replace("''", "'");
}
