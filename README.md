# PatchMemoryRelocationsModule 
Replaces the usage of `MEMAllocFromDefaultHeap`, `MEMAllocFromDefaultHeapEx` and `MEMFreeToDefaultHeap` with similar function from the MemoryMappingModule in the currently running modules.

# Usage
Run this module via [SetupPayload](https://github.com/wiiu-env/SetupPayload/), requires the [MemoryMappingModule](https://github.com/wiiu-env/MemoryMappingModule) to be running at the same time.

# Dependencies:
- [wut](https://github.com/decaf-emu/wut)
- [WUMS](https://github.com/wiiu-env/WiiUModuleSystem)
- [MemoryMappingModule](https://github.com/wiiu-env/MemoryMappingModule) & [libmappedmemory](https://github.com/wiiu-env/libmappedmemory)