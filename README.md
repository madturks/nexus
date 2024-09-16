# Nexus

Reliable UDP implementation using modern C++.

## Build using clang

```sh
meson setup tgt --wipe --native-file=.config/meson-clang
meson compile -C tgt/
```


## Build using gcc

```sh
meson setup tgt --wipe --native-file=.config/meson-gcc
meson compile -C tgt/
```