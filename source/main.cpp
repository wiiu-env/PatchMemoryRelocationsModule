#include <wums.h>

#include "logger.h"
#include "version.h"

#include <memory/mappedmemory.h>

#include <coreinit/cache.h>
#include <coreinit/debug.h>
#include <coreinit/dynload.h>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include <cstdlib>
#include <cstring>

#define VERSION "v0.1.3"

WUMS_MODULE_EXPORT_NAME("homebrew_patchmemoryrelocations");
WUMS_MODULE_INIT_BEFORE_RELOCATION_DONE_HOOK();
WUMS_MODULE_SKIP_INIT_FINI();

std::map<std::string, OSDynLoad_Module> gUsedRPLs;
std::vector<void *> gAllocatedAddresses;

WUMS_INTERNAL_HOOK_CLEAR_ALLOCATED_RPL_MEMORY() {
    gUsedRPLs.clear();
    // If an allocated rpl was not released properly (e.g. if something else calls OSDynload_Acquire without releasing it)
    // memory gets leaked. Let's clean this up!
    for (auto &addr : gAllocatedAddresses) {
        DEBUG_FUNCTION_LINE_WARN("Memory allocated by OSDynload was not freed properly, let's clean it up! (%08X)", addr);
        free((void *) addr);
    }
    gAllocatedAddresses.clear();
}

WUMS_APPLICATION_STARTS() {
    OSReport("Running PatchMemoryRelocationsModule " VERSION VERSION_EXTRA "\n");
}

bool elfLinkOne(char type, size_t offset, int32_t addend, uint32_t destination, uint32_t symbol_addr, relocation_trampoline_entry_t *trampoline_data, uint32_t trampoline_data_length,
                RelocationType reloc_type);

static uint32_t CustomDynLoadAlloc(int32_t size, int32_t align, void **outAddr) {
    if (!outAddr) {
        return OS_DYNLOAD_INVALID_ALLOCATOR_PTR;
    }

    if (align >= 0 && align < 4) {
        align = 4;
    } else if (align < 0 && align > -4) {
        align = -4;
    }

    if (!((*outAddr = MEMAllocFromMappedMemoryEx(size, align)))) {
        return OS_DYNLOAD_OUT_OF_MEMORY;
    }

    // keep track of allocated memory to clean it up in case the RPLs won't get unloaded properly
    gAllocatedAddresses.push_back(*outAddr);

    return OS_DYNLOAD_OK;
}

static void CustomDynLoadFree(void *addr) {
    MEMFreeToMappedMemory(addr);

    // Remove from list
    if (const auto it = std::ranges::find(gAllocatedAddresses, addr); it != gAllocatedAddresses.end()) {
        gAllocatedAddresses.erase(it);
    }
}

WUMS_INTERNAL_GET_CUSTOM_RPL_ALLOCATOR() {
    return {CustomDynLoadAlloc, CustomDynLoadFree};
}

WUMS_RELOCATIONS_DONE(args) {
    auto *gModuleData = args.module_information;
    if (gModuleData == nullptr) {
        OSFatal("PatchMemoryRelocations: Failed to get gModuleData pointer.");
    }
    if (gModuleData->version != MODULE_INFORMATION_VERSION) {
        OSFatal("PatchMemoryRelocations: The module information struct version does not match.");
    }

    initLogging();
    for (uint32_t i = 0; i < gModuleData->number_modules; i++) {
        auto *curModule = &gModuleData->modules[i];

        if (strcmp("homebrew_memorymapping", curModule->module_export_name) == 0 ||
            strcmp("homebrew_patchmemoryrelocations", curModule->module_export_name) == 0 ||
            strcmp("homebrew_functionpatcher", curModule->module_export_name) == 0) {
            DEBUG_FUNCTION_LINE_VERBOSE("Skip %s", curModule->module_export_name);
            continue;
        }

        DEBUG_FUNCTION_LINE_VERBOSE("Patch relocations of %s", curModule->module_export_name);
        for (uint32_t j = 0; j < curModule->number_linking_entries; j++) {
            auto linkingEntry = &curModule->linking_entries[j];

            uint32_t functionAddress = 0;
            // Skip the ".fimport_" part of the importName when comparing
            if (strcmp("coreinit", &linkingEntry->importEntry->importName[9]) == 0) {
                if (strcmp("MEMAllocFromDefaultHeap", linkingEntry->functionName) == 0) {
                    functionAddress = (uint32_t) &MEMAllocFromMappedMemory;
                } else if (strcmp("MEMAllocFromDefaultHeapEx", linkingEntry->functionName) == 0) {
                    functionAddress = (uint32_t) &MEMAllocFromMappedMemoryEx;
                } else if (strcmp("MEMFreeToDefaultHeap", linkingEntry->functionName) == 0) {
                    functionAddress = (uint32_t) &MEMFreeToMappedMemory;
                }

                if (functionAddress != 0) {
                    if (!elfLinkOne(linkingEntry->type, linkingEntry->offset, linkingEntry->addend, (uint32_t) linkingEntry->destination, functionAddress, nullptr, 0, RELOC_TYPE_IMPORT)) {
                        OSFatal("homebrew_patchmemoryrelocations: Relocation failed\n");
                    }
                }
            }
        }
    }
    deinitLogging();
}

#define R_PPC_NONE         0
#define R_PPC_ADDR32       1
#define R_PPC_ADDR16_LO    4
#define R_PPC_ADDR16_HI    5
#define R_PPC_ADDR16_HA    6
#define R_PPC_REL24        10
#define R_PPC_REL14        11
#define R_PPC_DTPMOD32     68
#define R_PPC_DTPREL32     78
#define R_PPC_GHS_REL16_HA 251
#define R_PPC_GHS_REL16_HI 252
#define R_PPC_GHS_REL16_LO 253

// See https://github.com/decaf-emu/decaf-emu/blob/43366a34e7b55ab9d19b2444aeb0ccd46ac77dea/src/libdecaf/src/cafe/loader/cafe_loader_reloc.cpp#L144
bool elfLinkOne(char type, size_t offset, int32_t addend, uint32_t destination, uint32_t symbol_addr, relocation_trampoline_entry_t *trampoline_data, uint32_t trampoline_data_length,
                RelocationType reloc_type) {
    if (type == R_PPC_NONE) {
        return true;
    }

    auto target = destination + offset;
    auto value  = symbol_addr + addend;

    auto relValue = value - static_cast<uint32_t>(target);

    switch (type) {
        case R_PPC_NONE:
            break;
        case R_PPC_ADDR32:
            *((uint32_t *) (target)) = value;
            break;
        case R_PPC_ADDR16_LO:
            *((uint16_t *) (target)) = static_cast<uint16_t>(value & 0xFFFF);
            break;
        case R_PPC_ADDR16_HI:
            *((uint16_t *) (target)) = static_cast<uint16_t>(value >> 16);
            break;
        case R_PPC_ADDR16_HA:
            *((uint16_t *) (target)) = static_cast<uint16_t>((value + 0x8000) >> 16);
            break;
        case R_PPC_DTPMOD32:
            DEBUG_FUNCTION_LINE_ERR("################IMPLEMENT ME");
            //*((int32_t *)(target)) = tlsModuleIndex;
            break;
        case R_PPC_DTPREL32:
            *((uint32_t *) (target)) = value;
            break;
        case R_PPC_GHS_REL16_HA:
            *((uint16_t *) (target)) = static_cast<uint16_t>((relValue + 0x8000) >> 16);
            break;
        case R_PPC_GHS_REL16_HI:
            *((uint16_t *) (target)) = static_cast<uint16_t>(relValue >> 16);
            break;
        case R_PPC_GHS_REL16_LO:
            *((uint16_t *) (target)) = static_cast<uint16_t>(relValue & 0xFFFF);
            break;
        case R_PPC_REL14: {
            auto distance = static_cast<int32_t>(value) - static_cast<int32_t>(target);
            if (distance > 0x7FFC || distance < -0x7FFC) {
                DEBUG_FUNCTION_LINE_ERR("***14-bit relative branch cannot hit target.");
                return false;
            }

            if (distance & 3) {
                DEBUG_FUNCTION_LINE_ERR("***RELOC ERROR %d: lower 2 bits must be zero before shifting.", -470040);
                return false;
            }

            if ((distance >= 0 && (distance & 0xFFFF8000)) ||
                (distance < 0 && ((distance & 0xFFFF8000) != 0xFFFF8000))) {
                DEBUG_FUNCTION_LINE_ERR("***RELOC ERROR %d: upper 17 bits before shift must all be the same.", -470040);
                return false;
            }

            *(int32_t *) target = (*(int32_t *) target & 0xFFBF0003) | (distance & 0x0000fffc);
            break;
        }
        case R_PPC_REL24: {
            // if (isWeakSymbol && !symbolValue) {
            //     symbolValue = static_cast<uint32_t>(target);
            //     value = symbolValue + addend;
            // }
            auto distance = static_cast<int32_t>(value) - static_cast<int32_t>(target);
            if (distance > 0x1FFFFFC || distance < -0x1FFFFFC) {
                if (trampoline_data == nullptr) {
                    DEBUG_FUNCTION_LINE_ERR("***24-bit relative branch cannot hit target. Trampoline isn't provided");
                    DEBUG_FUNCTION_LINE_ERR("***value %08X - target %08X = distance %08X", value, target, distance);
                    return false;
                } else {
                    relocation_trampoline_entry_t *freeSlot = nullptr;
                    for (uint32_t i = 0; i < trampoline_data_length; i++) {
                        // We want to override "old" relocations of imports
                        // Pending relocations have the status RELOC_TRAMP_IMPORT_IN_PROGRESS.
                        // When all relocations are done successfully, they will be turned into RELOC_TRAMP_IMPORT_DONE
                        // so they can be overridden/updated/reused on the next application launch.
                        //
                        // Relocations that won't change will have the status RELOC_TRAMP_FIXED and are set to free when the module is unloaded.
                        if (trampoline_data[i].status == RELOC_TRAMP_FREE ||
                            trampoline_data[i].status == RELOC_TRAMP_IMPORT_DONE) {
                            freeSlot = &(trampoline_data[i]);
                            break;
                        }
                    }
                    if (freeSlot == nullptr) {
                        DEBUG_FUNCTION_LINE_ERR("***24-bit relative branch cannot hit target. Trampoline data list is full");
                        DEBUG_FUNCTION_LINE_ERR("***value %08X - target %08X = distance %08X", value, target, target - (uint32_t) & (freeSlot->trampoline[0]));
                        return false;
                    }
                    auto symbolValue = (uint32_t) & (freeSlot->trampoline[0]);
                    auto newValue    = symbolValue + addend;
                    auto newDistance = static_cast<int32_t>(newValue) - static_cast<int32_t>(target);
                    if (newDistance > 0x1FFFFFC || newDistance < -0x1FFFFFC) {
                        DEBUG_FUNCTION_LINE_ERR("**Cannot link 24-bit jump (too far to tramp buffer).");
                        if (newDistance < 0) {
                            DEBUG_FUNCTION_LINE_ERR("***value %08X - target %08X = distance -%08X", newValue, target, abs(newDistance));
                        } else {
                            DEBUG_FUNCTION_LINE_ERR("***value %08X - target %08X = distance  %08X", newValue, target, newDistance);
                        }
                        return false;
                    }

                    freeSlot->trampoline[0] = 0x3D600000 | ((((uint32_t) value) >> 16) & 0x0000FFFF); // lis r11, real_addr@h
                    freeSlot->trampoline[1] = 0x616B0000 | (((uint32_t) value) & 0x0000ffff);         // ori r11, r11, real_addr@l
                    freeSlot->trampoline[2] = 0x7D6903A6;                                             // mtctr   r11
                    freeSlot->trampoline[3] = 0x4E800420;                                             // bctr
                    DCFlushRange((void *) freeSlot->trampoline, sizeof(freeSlot->trampoline));
                    ICInvalidateRange((unsigned char *) freeSlot->trampoline, sizeof(freeSlot->trampoline));

                    if (reloc_type == RELOC_TYPE_FIXED) {
                        freeSlot->status = RELOC_TRAMP_FIXED;
                    } else {
                        // Relocations for the imports may be overridden
                        freeSlot->status = RELOC_TRAMP_IMPORT_DONE;
                    }
                    distance = newDistance;
                }
            }

            if (distance & 3) {
                DEBUG_FUNCTION_LINE_ERR("***RELOC ERROR %d: lower 2 bits must be zero before shifting.", -470022);
                return false;
            }

            if (distance < 0 && (distance & 0xFE000000) != 0xFE000000) {
                DEBUG_FUNCTION_LINE_ERR("***RELOC ERROR %d: upper 7 bits before shift must all be the same (1).", -470040);
                return false;
            }

            if (distance >= 0 && (distance & 0xFE000000)) {
                DEBUG_FUNCTION_LINE_ERR("***RELOC ERROR %d: upper 7 bits before shift must all be the same (0).", -470040);
                return false;
            }

            *(int32_t *) target = (*(int32_t *) target & 0xfc000003) | (distance & 0x03fffffc);
            break;
        }
        default:
            DEBUG_FUNCTION_LINE_ERR("***ERROR: Unsupported Relocation_Add Type (%08X):", type);
            return false;
    }
    ICInvalidateRange(reinterpret_cast<void *>(target), 4);
    DCFlushRange(reinterpret_cast<void *>(target), 4);
    return true;
}
