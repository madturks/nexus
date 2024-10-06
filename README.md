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

### Start MSQUIC LTTng trace

Install the lttng runtime dependencies
```
sudo apt-get install liblttng-ust-dev liblttng-ctl-dev
```

Export the following environment variables
```
export LTTNG_UST_DEBUG=1
export LTTNG_UST_REGISTER_TIMEOUT=-1
```

Then:

```
mkdir msquic_lttng
lttng create msquic -o=./msquic_lttng
lttng enable-event --userspace "CLOG_*"
lttng add-context --userspace --type=vpid --type=vtid
lttng start
# lttng stop msquic
```

### Format the project

```
# https://mesonbuild.com/Code-formatting.html
ninja -C tgt clang-format
```


### 

https://packages.microsoft.com/ubuntu/24.04/prod/pool/main/libm/libmsquic/