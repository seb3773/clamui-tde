#include "tqtbinaryserializer.h"

#include <ntqbuffer.h>
#include <ntqdatastream.h>

#include "tqtbytearrayutil.h"

TQVariant TQtBinarySerializer::fromString(const TQString& value, const TQVariant::Type type) const {
    (void)type;

    const TQByteArray in = TQtByteArrayUtil::fromLatin1(value);
    const TQByteArray raw = TQtByteArrayUtil::base64Decode(in);

    TQBuffer buf;
    buf.setBuffer(raw);
    if (!buf.open(IO_ReadOnly)) return TQVariant();

    TQDataStream ds(&buf);
    TQVariant v;
    ds >> v;

    return v;
}

TQString TQtBinarySerializer::toString(const TQVariant& value) const {
    TQByteArray raw;

    TQBuffer buf;
    buf.setBuffer(raw);
    if (!buf.open(IO_WriteOnly)) return TQString();

    TQDataStream ds(&buf);
    ds << value;

    const TQByteArray b64 = TQtByteArrayUtil::base64Encode(raw);
    return TQtByteArrayUtil::toLatin1String(b64);
}
