# CMake


```sh
cmake -DPICO_SDK_PATH=/home/hv-admin/repos/pico-sdk -DPICO_BOARD=pico2 -DPICO_PLATFORM=rp2350  ..
```

libpng, zlib は手動で取り込んだ方が手っ取り早そう.

```
-DBUILD_SHARED_LIBS=off -DPNG_SHARED=off -DPNG_TESTS=off -DZLIB_ROOT=/home/hv-admin/repos/pico/prog01/libs/zlib/zlib-1.3.1
```
