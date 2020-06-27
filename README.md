# PatchMemoryRelocationsModule 
Replaces the usage of `MEMAllocFromDefaultHeap`, `MEMAllocFromDefaultHeapEx` and `MEMFreeToDefaultHeap` with similar function from the MemoryMappingModule in the currently running modules.

# Usage
Run this module via [SetupPayload](https://github.com/wiiu-env/SetupPayload/), requires the [MemoryMappingModule](https://github.com/wiiu-env/MemoryMappingModule) to be running at the same time.

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

# Dependencies:
- [wut](https://github.com/decaf-emu/wut)
- [WUMS](https://github.com/wiiu-env/WiiUModuleSystem)
- [MemoryMappingModule](https://github.com/wiiu-env/MemoryMappingModule) & [libmappedmemory](https://github.com/wiiu-env/libmappedmemory)