# json/example

This is a small test program for the `json/` mini-lib.

## Build

You must compile Parson and the wrapper together.

Example (adjust include/lib paths for your TQt3/TDE install):

```sh
g++ -O2 -std=c++03 \
  -I.. \
  -o tqtjson_example \
  main.cpp ../tqtjson.cpp ../parson.c \
  $(tde-config --cflags --libs 2>/dev/null)
```

If `tde-config` is not available, replace it with the right flags, typically including the TQt headers and linking against `-ltqt` (and possibly `-ltdecore` depending on your setup).

## Run

```sh
./tqtjson_example
```
