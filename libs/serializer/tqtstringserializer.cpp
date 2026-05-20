#include "tqtstringserializer.h"

#include <ntqdatetime.h>
#include <ntqvaluelist.h>

#include <stdio.h>

static int tqt_parse_iso_date(const TQString& s, TQDate* out) {
    if (!out) return -1;
    int y = 0, m = 0, d = 0;
    if (sscanf(s.latin1(), "%d-%d-%d", &y, &m, &d) != 3) return -1;
    const TQDate dt(y, m, d);
    if (!dt.isValid()) return -1;
    *out = dt;
    return 0;
}

static int tqt_parse_iso_time(const TQString& s, TQTime* out) {
    if (!out) return -1;
    int hh = 0, mm = 0, ss = 0, ms = 0;

    const char* p = s.latin1();
    if (!p) return -1;

    int n = sscanf(p, "%d:%d:%d.%d", &hh, &mm, &ss, &ms);
    if (n < 3) {
        ms = 0;
        n = sscanf(p, "%d:%d:%d", &hh, &mm, &ss);
        if (n != 3) return -1;
    }

    const TQTime tm(hh, mm, ss, ms);
    if (!tm.isValid()) return -1;
    *out = tm;
    return 0;
}

static int tqt_parse_iso_datetime(const TQString& s, TQDateTime* out) {
    if (!out) return -1;

    int sp = s.find(' ');
    if (sp < 0) sp = s.find('T');
    if (sp < 0) return -1;

    const TQString ds = s.left(sp);
    const TQString ts = s.mid(sp + 1);

    TQDate d;
    TQTime t;
    if (tqt_parse_iso_date(ds, &d) != 0) return -1;
    if (tqt_parse_iso_time(ts, &t) != 0) return -1;

    *out = TQDateTime(d, t);
    return out->isValid() ? 0 : -1;
}

static inline TQByteArray tqt_ba_from_utf8(const TQString& s) {
    const TQCString u = s.utf8();
    TQByteArray b;
    if (!u.data() || u.length() <= 0) return b;
    b.duplicate(u.data(), (uint)u.length());
    return b;
}

static inline TQString tqt_str_from_utf8(const TQByteArray& b) {
    if (b.isEmpty()) return TQString();
    return TQString::fromUtf8(b.data(), (int)b.size());
}

TQVariant TQtStringSerializer::fromString(const TQString& value, const TQVariant::Type type) const {
    switch (type) {
        case TQVariant::Bool:
            return TQVariant((bool)(value.lower() == TQString("true") || value == TQString("1")));

        case TQVariant::Int:
            return TQVariant(value.toInt());
        case TQVariant::UInt:
            return TQVariant(value.toUInt());
        case TQVariant::LongLong:
            return TQVariant(value.toLongLong());
        case TQVariant::ULongLong:
            return TQVariant(value.toULongLong());
        case TQVariant::Double:
            return TQVariant(value.toDouble());

        case TQVariant::String:
            return TQVariant(unescapeString(value));

        case TQVariant::StringList: {
            TQStringList ret;
            TQString copy(value);
            TQString out;
            while (readString(copy, out)) ret.append(out);
            return TQVariant(ret);
        }

        case TQVariant::Date: {
            TQDate d;
            if (tqt_parse_iso_date(value, &d) != 0) return TQVariant(TQDate());
            return TQVariant(d);
        }
        case TQVariant::Time: {
            TQTime t;
            if (tqt_parse_iso_time(value, &t) != 0) return TQVariant(TQTime());
            return TQVariant(t);
        }
        case TQVariant::DateTime: {
            TQDateTime dt;
            if (tqt_parse_iso_datetime(value, &dt) != 0) return TQVariant(TQDateTime());
            return TQVariant(dt);
        }

        case TQVariant::Point: {
            const TQStringList parts = TQStringList::split(TQString(","), value);
            if (parts.count() != 2) return TQVariant(TQPoint());
            return TQVariant(TQPoint(parts[0].toInt(), parts[1].toInt()));
        }

        case TQVariant::Size: {
            const TQStringList parts = TQStringList::split(TQString(","), value);
            if (parts.count() != 2) return TQVariant(TQSize());
            return TQVariant(TQSize(parts[0].toInt(), parts[1].toInt()));
        }

        case TQVariant::Rect: {
            const TQStringList parts = TQStringList::split(TQString(","), value);
            if (parts.count() != 4) return TQVariant(TQRect());
            return TQVariant(TQRect(parts[0].toInt(), parts[1].toInt(), parts[2].toInt(), parts[3].toInt()));
        }

        case TQVariant::ByteArray:
            return TQVariant(tqt_ba_from_utf8(value));

        default:
            break;
    }

    return TQVariant();
}

TQString TQtStringSerializer::toString(const TQVariant& value) const {
    switch (value.type()) {
        case TQVariant::Bool:
        case TQVariant::Int:
        case TQVariant::UInt:
        case TQVariant::LongLong:
        case TQVariant::ULongLong:
        case TQVariant::Double:
            return value.toString();

        case TQVariant::String:
            return value.toString();

        case TQVariant::StringList: {
            TQString ret;
            const TQStringList sl = value.toStringList();
            for (TQStringList::ConstIterator it = sl.begin(); it != sl.end(); ++it) {
                if (!ret.isEmpty()) ret.append(" ");
                ret.append(TQString("\"") + escapeString(*it) + TQString("\""));
            }
            return ret;
        }

        case TQVariant::Date:
            return value.toDate().toString(TQt::ISODate);
        case TQVariant::Time:
            return value.toTime().toString(TQt::ISODate);
        case TQVariant::DateTime:
            return value.toDateTime().toString(TQt::ISODate);

        case TQVariant::Point: {
            const TQPoint pt = value.toPoint();
            TQValueList<TQVariant> l;
            l.append(TQVariant(pt.x()));
            l.append(TQVariant(pt.y()));
            return fromVariantList(l);
        }

        case TQVariant::Size: {
            const TQSize sz = value.toSize();
            TQValueList<TQVariant> l;
            l.append(TQVariant(sz.width()));
            l.append(TQVariant(sz.height()));
            return fromVariantList(l);
        }

        case TQVariant::Rect: {
            const TQRect rc = value.toRect();
            TQValueList<TQVariant> l;
            l.append(TQVariant(rc.x()));
            l.append(TQVariant(rc.y()));
            l.append(TQVariant(rc.width()));
            l.append(TQVariant(rc.height()));
            return fromVariantList(l);
        }

        case TQVariant::ByteArray:
            return tqt_str_from_utf8(value.toByteArray());

        default:
            break;
    }

    return TQString();
}

TQString TQtStringSerializer::fromVariantList(const TQValueList<TQVariant>& list) const {
    TQString ret;
    for (TQValueList<TQVariant>::ConstIterator it = list.begin(); it != list.end(); ++it) {
        if (!ret.isEmpty()) ret.append(",");
        ret.append((*it).toString());
    }
    return ret;
}

#define CASE_W(o, r) \
    case o:          \
        ret.append(TQString(r)); \
        break;

TQString TQtStringSerializer::escapeString(const TQString& str) const {
    TQString ret;
    for (int i = 0; i < (int)str.length(); ++i) {
        switch (str.at(i).cell()) {
            CASE_W('\\', "\\\\")
            CASE_W('\r', "\\r")
            CASE_W('\n', "\\n")
            CASE_W('\a', "\\a")
            CASE_W('\b', "\\b")
            CASE_W('\f', "\\f")
            CASE_W('\t', "\\t")
            CASE_W('\v', "\\v")
            CASE_W('\"', "\\\"")
            default:
                ret.append(str.at(i));
        }
    }
    return ret;
}

TQString TQtStringSerializer::unescapeString(const TQString& str) const {
    TQString ret;
    for (int i = 0; i < (int)str.length(); ++i) {
        if (str.at(i) == '\\' && (int)str.length() > i) {
            switch (str.at(++i).cell()) {
                case '\\':
                    ret.append(TQString("\\"));
                    break;
                case 'r':
                    ret.append(TQString("\r"));
                    break;
                case 'n':
                    ret.append(TQString("\n"));
                    break;
                case 'a':
                    ret.append(TQString("\a"));
                    break;
                case 'b':
                    ret.append(TQString("b"));
                    break;
                case 'f':
                    ret.append(TQString("\f"));
                    break;
                case 't':
                    ret.append(TQString("\t"));
                    break;
                case 'v':
                    ret.append(TQString("\v"));
                    break;
                case '"':
                    ret.append(TQString("\\\""));
                    break;
                default:
                    ret.append(str.at(i));
            }
        } else {
            ret.append(str.at(i));
        }
    }
    return ret;
}

bool TQtStringSerializer::readString(TQString& text, TQString& out) const {
    int start = -1;
    int end = -1;

    for (int i = 0; i < (int)text.length(); ++i) {
        if (text.at(i) == '"' && (i == 0 || text.at(i - 1) != '\\')) {
            if (start == -1)
                start = i;
            else
                end = i;
        }
        if (end != -1) {
            out = text.mid(start + 1, end - start - 1);
            text = text.mid(end + 1);
            return true;
        }
    }
    return false;
}
