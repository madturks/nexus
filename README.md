# Nexus

Reliable UDP implementation using modern C++.

## Build using clang

```sh
clear && cd /workspaces/nexus && meson setup tgt --wipe --native-file=.config/meson-clang && clear && cd tgt && meson compile
```


## Build using gcc

```sh
clear && cd /workspaces/nexus && meson setup tgt --wipe --native-file=.config/meson-gcc && clear && cd tgt && meson compile
```


### Running TSAN

If you're getting `ADDR_NO_RANDOMIZE` errors running TSAN-enabled executables, try disabling address space randomization:


```sh
echo 0 | sudo tee /proc/sys/kernel/randomize_va_space
```

### Running LSAN

```sh
ASAN_OPTIONS=detect_leaks=1 ./command
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

