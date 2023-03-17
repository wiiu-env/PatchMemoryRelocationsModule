[![CI-Release](https://github.com/wiiu-env/PatchMemoryRelocationsModule/actions/workflows/ci.yml/badge.svg)](https://github.com/wiiu-env/PatchMemoryRelocationsModule/actions/workflows/ci.yml)

# PatchMemoryRelocationsModule 
Replaces the usage of `MEMAllocFromDefaultHeap`, `MEMAllocFromDefaultHeapEx` and `MEMFreeToDefaultHeap` with similar function from the MemoryMappingModule in the currently running modules.

## Usage
(`[ENVIRONMENT]` is a placeholder for the actual environment name.)

1. Copy the file `PatchMemoryRelocationsModule.wms` into `sd:/wiiu/environments/[ENVIRONMENT]/modules`.  
2. Requires the [WUMSLoader](https://github.com/wiiu-env/WUMSLoader) in `sd:/wiiu/environments/[ENVIRONMENT]/modules/setup`.

## Buildflags

### Logging
Building via `make` only logs errors (via OSReport). To enable logging via the [LoggingModule](https://github.com/wiiu-env/LoggingModule) set `DEBUG` to `1` or `VERBOSE`.

`make` Logs errors only (via OSReport).  
`make DEBUG=1` Enables information and error logging via [LoggingModule](https://github.com/wiiu-env/LoggingModule).  
`make DEBUG=VERBOSE` Enables verbose information and error logging via [LoggingModule](https://github.com/wiiu-env/LoggingModule).

## Building using the Dockerfile

It's possible to use a docker image for building. This way you don't need anything installed on your host system.

```
# Build docker image (only needed once)
docker build . -t patchmemoryrelocationsmodule-builder

# make 
docker run -it --rm -v ${PWD}:/project patchmemoryrelocationsmodule-builder make

# make clean
docker run -it --rm -v ${PWD}:/project patchmemoryrelocationsmodule-builder make clean
```

## Format the code via docker

`docker run --rm -v ${PWD}:/src ghcr.io/wiiu-env/clang-format:13.0.0-2 -r ./source -i`

# Dependencies:
- [wut](https://github.com/decaf-emu/wut)
- [WUMS](https://github.com/wiiu-env/WiiUModuleSystem)
- [MemoryMappingModule](https://github.com/wiiu-env/MemoryMappingModule) & [libmappedmemory](https://github.com/wiiu-env/libmappedmemory)