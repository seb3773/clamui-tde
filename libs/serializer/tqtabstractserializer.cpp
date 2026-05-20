#include "tqtabstractserializer.h"

TQVariant TQtAbstractSerializer::deserialize(const TQString& value, const TQVariant::Type type) const {
    return fromString(unescapeString(value), type);
}

TQString TQtAbstractSerializer::serialize(const TQVariant& value) const {
    return escapeString(toString(value));
}

TQString TQtAbstractSerializer::escapeString(const TQString& str) const {
    return str;
}

TQString TQtAbstractSerializer::unescapeString(const TQString& str) const {
    return str;
}
