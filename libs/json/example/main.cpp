#include "../tqtjson.h"

#include <ntqstring.h>
#include <ntqmap.h>
#include <ntqvariant.h>
#include <ntqvaluelist.h>

#include <stdio.h>

static void print_variant(const TQVariant& v, int depth);

static void indent(int depth) {
    for (int i = 0; i < depth; ++i) putchar(' ');
}

static void print_map(const TQMap<TQString, TQVariant>& m, int depth) {
    indent(depth);
    puts("{");

    for (TQMap<TQString, TQVariant>::ConstIterator it = m.begin(); it != m.end(); ++it) {
        indent(depth + 2);
        printf("%s: ", it.key().utf8().data());
        print_variant(it.data(), depth + 2);
    }

    indent(depth);
    puts("}");
}

static void print_list(const TQValueList<TQVariant>& l, int depth) {
    indent(depth);
    puts("[");

    for (TQValueList<TQVariant>::ConstIterator it = l.begin(); it != l.end(); ++it) {
        print_variant(*it, depth + 2);
    }

    indent(depth);
    puts("]");
}

static void print_variant(const TQVariant& v, int depth) {
    if (!v.isValid() || v.isNull()) {
        indent(depth);
        puts("null");
        return;
    }

    switch (v.type()) {
        case TQVariant::Bool:
            indent(depth);
            puts(v.toBool() ? "true" : "false");
            return;
        case TQVariant::Double:
            indent(depth);
            printf("%f\n", v.toDouble());
            return;
        case TQVariant::Int:
            indent(depth);
            printf("%d\n", v.toInt());
            return;
        case TQVariant::UInt:
            indent(depth);
            printf("%u\n", v.toUInt());
            return;
        case TQVariant::LongLong:
            indent(depth);
            printf("%lld\n", (long long)v.toLongLong());
            return;
        case TQVariant::ULongLong:
            indent(depth);
            printf("%llu\n", (unsigned long long)v.toULongLong());
            return;
        case TQVariant::String:
            indent(depth);
            printf("\"%s\"\n", v.toString().utf8().data());
            return;
        case TQVariant::List:
            print_list(v.toList(), depth);
            return;
        case TQVariant::Map:
            print_map(v.toMap(), depth);
            return;
        default:
            indent(depth);
            printf("<variant type=%d>\n", (int)v.type());
            return;
    }
}

int main() {
    const TQString js = "{\"a\":1,\"b\":[true,\"x\"],\"obj\":{\"k\":\"v\"}}";

    TQtJsonParseError err;
    TQtJsonDocument doc = TQtJsonDocument::fromJson(js, &err);
    if (!err.error.isEmpty()) {
        fprintf(stderr, "parse error: %s\n", err.error.utf8().data());
        return 1;
    }

    if (!doc.isObject()) {
        fprintf(stderr, "expected object\n");
        return 1;
    }

    TQtJsonObject o = doc.object();

    double a = o.value("a").toDouble(-1.0);
    printf("a=%f\n", a);

    TQtJsonArray b = o.value("b").toArray();
    printf("b.size=%d\n", b.size());
    printf("b[0]=%s\n", b.at(0).toBool() ? "true" : "false");
    printf("b[1]=%s\n", b.at(1).toString("?").utf8().data());

    TQVariant v = doc.value().toVariant();
    puts("\nAs variant:");
    print_variant(v, 0);

    TQString out_pretty = doc.toJson(true);
    puts("\nSerialized pretty:");
    puts(out_pretty.utf8().data());

    TQtJsonObject newobj = TQtJsonObject::empty();
    newobj.insert("hello", TQtJsonValue::fromString("world"));
    newobj.insert("num", TQtJsonValue::fromDouble(3.14));
    newobj.insert("orig", doc.value());

    TQtJsonDocument newdoc = TQtJsonDocument::fromValue(TQtJsonValue::fromVariant(TQVariant(newobj.toVariantMap())));
    puts("\nProgrammatic build:");
    puts(newdoc.toJson(true).utf8().data());

    return 0;
}
