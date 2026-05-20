# concurrent/example

Minimal threadpool + future test.

## Build (manual)

```sh
tqmoc -o main.moc main.cpp

g++ -O2 -std=c++03 \
  -I.. \
  -o tqtconcurrent_example \
  main.cpp ../tqtconcurrent.cpp \
  $(pkg-config --cflags --libs tqt-mt) -lpthread
```

## Run

```sh
./tqtconcurrent_example
```
