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


### Running TSAN

If you're getting `ADDR_NO_RANDOMIZE` errors running TSAN-enabled executables, try disabling address space randomization:


```sh
echo 0 | sudo tee /proc/sys/kernel/randomize_va_space
```


### 

https://packages.microsoft.com/ubuntu/24.04/prod/pool/main/libm/libmsquic/