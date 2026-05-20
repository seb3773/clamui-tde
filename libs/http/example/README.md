# http/example

This is a minimal async HTTP GET test.

## Build

You need:

- TQt3 headers/libs
- libcurl
- moc (from TQt3)

Example command (adjust for your environment):

```sh
moc -o main.moc main.cpp

g++ -O2 -std=c++03 \
  -I.. \
  -o tqthttp_example \
  main.cpp ../tqtnetwork.cpp \
  -lcurl -lpthread \
  $(tde-config --cflags --libs 2>/dev/null)
```

## Run

```sh
./tqthttp_example https://example.com/
```
