# json

Files:

- `tqtjson.h` / `tqtjson.cpp`: C++ wrapper (TQt3) on top of Parson.
- Backend: `parson.c` and `parson.h`.

Main classes:

- `TQtJsonDocument`: parse + serialize.
- `TQtJsonValue`: generic JSON value (null/bool/number/string/object/array).
- `TQtJsonObject`: object helper.
- `TQtJsonArray`: array helper.

Usage example:

```cpp
#include "tqtjson.h"

TQtJsonParseError err;
TQtJsonDocument doc = TQtJsonDocument::fromJson("{\"a\":1,\"b\":[true,\"x\"]}", &err);
if (!err.error.isEmpty()) {
    // handle error
}

if (doc.isObject()) {
    TQtJsonObject o = doc.object();
    double a = o.value("a").toDouble();
    TQtJsonArray b = o.value("b").toArray();
    bool b0 = b.at(0).toBool();
    TQString b1 = b.at(1).toString();
}

TQString out = doc.toJson(true);
```

Variant conversion:

- `TQtJsonValue::fromVariant(v)` builds a JSON value from a `TQVariant` (maps/lists supported).
- `TQtJsonValue::toVariant()` converts parsed JSON to `TQVariant` (object->map, array->list).
