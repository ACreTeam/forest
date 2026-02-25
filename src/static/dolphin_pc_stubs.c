/**
 * PC stub implementations for Dolphin SDK symbols used by jaudio_NES and others.
 * The real Dolphin SDK is GameCube/PowerPC-specific (inline asm, hardware regs).
 * These stubs allow the PC build to link; audio streaming may be no-op until
 * proper PC implementations exist.
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <dolphin/types.h>
#include <dolphin/os/OSAlloc.h>
#include <dolphin/os/OSInterrupt.h>
#include <dolphin/os/OSCache.h>
#include <dolphin/os/OSArena.h>
#include <dolphin/os/OSThread.h>
#include <dolphin/os/OSMutex.h>
#include <dolphin/os/OSMessage.h>
#include <dolphin/os/OSTime.h>
#include <dolphin/os/OSReset.h>
#include <dolphin/os/OSAlarm.h>
#include <dolphin/os/OSContext.h>
#include <dolphin/os/OSMemory.h>
#include <dolphin/os/OSError.h>
#include <dolphin/os/OSLink.h>
#include <dolphin/os/OSRtc.h>
#include <dolphin/os/OSFont.h>
#include <dolphin/ai.h>
#include <dolphin/dvd.h>
#include <dolphin/vi.h>
#include <dolphin/ar.h>
#include <dolphin/pad.h>
#include <dolphin/exi.h>
#include <dolphin/card.h>
#include <dolphin/base/PPCArch.h>
#include <GBA/gba.h>
#include <GBA/GBAPriv.h>
#include <jaudio_NES/dummyprobe.h>

#ifdef TARGET_PC

/* Dummy for boot.c search_partial_address / LoadLink (REL module); NULL = no modules */
OSModuleHeader* BaseModule = NULL;
GBAControl __GBA[4];
BOOL __GBAReset = FALSE;
CARDControl __CARDBlock[2];

/* Arena for OSGetArenaHi/Lo (dummy range) */
static char arena_lo[1];
static char arena_hi[1];

/* -------------------------------------------------------------------------- */
/* libforest ReconfigBATs (PowerPC BAT setup; no-op on PC)                     */
/* -------------------------------------------------------------------------- */
void ReconfigBATs(void)
{
}

/* -------------------------------------------------------------------------- */
/* OS interrupts                                                              */
/* -------------------------------------------------------------------------- */
BOOL OSDisableInterrupts(void) { return FALSE; }
BOOL OSEnableInterrupts(void) { return FALSE; }
BOOL OSRestoreInterrupts(BOOL level) { (void)level; return FALSE; }

/* -------------------------------------------------------------------------- */
/* OS init / alarm                                                            */
/* -------------------------------------------------------------------------- */
void OSInitAlarm(void) {}

void OSCreateAlarm(OSAlarm* a) { (void)a; }
void OSCancelAlarm(OSAlarm* a) { (void)a; }
void OSSetAlarm(OSAlarm* a, OSTime tick, OSAlarmHandler h) { (void)a;(void)tick;(void)h; }

void OSClearContext(OSContext* c) { (void)c; }
void OSSetCurrentContext(OSContext* c) { (void)c; }

BOOL OSJamMessage(OSMessageQueue* q, OSMessage msg, int flags) { (void)q;(void)msg;(void)flags; return TRUE; }

void OSSleepThread(OSThreadQueue* q) { (void)q; }
s32 OSSuspendThread(OSThread* t) { (void)t; return 0; }
OSPriority OSGetThreadPriority(OSThread* t) { (void)t; return 0; }
void OSInitThreadQueue(OSThreadQueue* q) { (void)q; }

__OSInterruptHandler __OSSetInterruptHandler(__OSInterrupt intr, __OSInterruptHandler h) { (void)intr;(void)h; return NULL; }
u32 __OSUnmaskInterrupts(u32 mask) { (void)mask; return 0; }

void DCFlushRange(void* addr, u32 size) { (void)addr;(void)size; }
void DCFlushRangeNoSync(void* addr, u32 size) { (void)addr;(void)size; }
void DCStoreRangeNoSync(void* addr, u32 size) { (void)addr;(void)size; }
void DCTouchRange(void* addr, u32 len) { (void)addr;(void)len; }
void DCZeroRange(void* addr, u32 size) { (void)addr;(void)size; }

void DVDInit(void) {}
BOOL DVDCheckDisk(void) { return FALSE; }
BOOL DVDCloseDir(DVDDir* dir) { (void)dir; return TRUE; }
s32 DVDConvertPathToEntrynum(const char* path) { (void)path; return -1; }
BOOL DVDOpenDir(const char* dirName, DVDDir* dir) { (void)dirName;(void)dir; return FALSE; }
BOOL DVDReadDir(DVDDir* dir, DVDDirEntry* entry) { (void)dir;(void)entry; return FALSE; }

typedef void (*VIRetraceCallback)(u32);
VIRetraceCallback VISetPreRetraceCallback(VIRetraceCallback cb) { (void)cb; return NULL; }
VIRetraceCallback VISetPostRetraceCallback(VIRetraceCallback cb) { (void)cb; return NULL; }

void PPCSync(void) {}
void LCDisable(void) {}

u32 ARGetBaseAddress(void) { return 0; }

AIDCallback AIRegisterDMACallback(AIDCallback cb) { (void)cb; return NULL; }
void AISetDSPSampleRate(u32 rate) { (void)rate; }
void AIStartDMA(void) {}

/* DSP (avoid pulling in dsp.h which uses vu32) */
u32 DSPCheckMailFromDSP(void) { return 0; }
u32 DSPCheckMailToDSP(void) { return 0; }
u32 DSPReadMailFromDSP(void) { return 0; }
void DSPSendMailToDSP(u32 mail) { (void)mail; }

/* -------------------------------------------------------------------------- */
/* OS thread                                                                  */
/* -------------------------------------------------------------------------- */
u32 OSGetStackPointer(void) { return 0; }
OSThread* OSGetCurrentThread(void) { return NULL; }
void OSExitThread(void* val)
{
    (void)val;
    fflush(stdout);
    abort();
}
BOOL OSSetThreadPriority(OSThread* t, OSPriority p) { (void)t; (void)p; return TRUE; }
s32 OSEnableScheduler(void) { return 0; }
void OSYieldThread(void) {}

BOOL OSCreateThread(OSThread* thread, void* (*func)(void*), void* param,
    void* stack, u32 stackSize, OSPriority priority, u16 attr)
{
    (void)thread; (void)func; (void)param; (void)stack; (void)stackSize;
    (void)priority; (void)attr;
    return FALSE;
}
s32 OSResumeThread(OSThread* thread) { (void)thread; return 0; }
BOOL OSJoinThread(OSThread* thread, void** val)
{
    (void)thread;
    if (val) {
        *val = NULL;
    }
    return TRUE;
}
BOOL OSIsThreadTerminated(OSThread* thread) { (void)thread; return TRUE; }
void OSDetachThread(OSThread* thread) { (void)thread; }
void OSCancelThread(OSThread* thread) { (void)thread; }

/* -------------------------------------------------------------------------- */
/* OS mutex                                                                    */
/* -------------------------------------------------------------------------- */
void OSInitMutex(OSMutex* mutex) { (void)mutex; memset(mutex, 0, sizeof(OSMutex)); }
void OSLockMutex(OSMutex* mutex) { (void)mutex; }
void OSUnlockMutex(OSMutex* mutex) { (void)mutex; }

/* -------------------------------------------------------------------------- */
/* OS message queue                                                           */
/* -------------------------------------------------------------------------- */
void OSInitMessageQueue(OSMessageQueue* queue, OSMessage* msgArray, int msgCount)
{
    (void)queue; (void)msgArray; (void)msgCount;
    if (queue) memset(queue, 0, sizeof(OSMessageQueue));
}
BOOL OSSendMessage(OSMessageQueue* queue, OSMessage msg, int flags)
{
    (void)queue; (void)msg; (void)flags;
    return TRUE;
}
BOOL OSReceiveMessage(OSMessageQueue* queue, OSMessage* msgPtr, int flags)
{
    (void)queue; (void)msgPtr; (void)flags;
    return FALSE;
}

/* -------------------------------------------------------------------------- */
/* OS reset / console                                                         */
/* -------------------------------------------------------------------------- */
u32 OSGetResetCode(void) { return 0; }
void OSResetSystem(int reset, u32 resetCode, BOOL forceMenu)
{
    (void)reset; (void)resetCode; (void)forceMenu;
}
BOOL OSGetResetSwitchState(void) { return FALSE; }
void OSGetSaveRegion(void** start, void** end) { if (start) *start = NULL; if (end) *end = NULL; }
u32 OSGetConsoleType(void) { return 0; }

/* -------------------------------------------------------------------------- */
/* OS RTC / font / link                                                        */
/* -------------------------------------------------------------------------- */
u32 OSGetProgressiveMode(void) { return 0; }
void OSSetProgressiveMode(u32 on) { (void)on; }
void __OSSetBootMode(u8 mode) { (void)mode; }
BOOL __OSSyncSram(void) { return TRUE; }
u16 OSGetFontEncode(void) { return 0; }
void OSSetStringTable(const void* string_table) { (void)string_table; }
BOOL OSLink(OSModuleInfo* newModule, void* bss) { (void)newModule; (void)bss; return TRUE; }
BOOL OSUnlink(OSModuleInfo* module) { (void)module; return TRUE; }

/* -------------------------------------------------------------------------- */
/* OS error / context / memory                                                 */
/* -------------------------------------------------------------------------- */
OSErrorHandler OSSetErrorHandler(OSError error, OSErrorHandler handler)
{
    (void)error; (void)handler;
    return NULL;
}
void OSFillFPUContext(OSContext* context) { (void)context; }
void OSProtectRange(u32 chan, void* addr, u32 nBytes, u32 control)
{
    (void)chan; (void)addr; (void)nBytes; (void)control;
}
u32 OSGetConsoleSimulatedMemSize(void) { return 0x2000000; /* 32MB */ }

/* -------------------------------------------------------------------------- */
/* Cache                                                                      */
/* -------------------------------------------------------------------------- */
void DCStoreRange(void* addr, u32 size) { (void)addr; (void)size; }
void DCInvalidateRange(void* addr, u32 size) { (void)addr; (void)size; }

/* -------------------------------------------------------------------------- */
/* PowerPC (no-op on PC)                                                      */
/* -------------------------------------------------------------------------- */
u32 PPCMfmsr(void) { return 0; }
void PPCMtmsr(u32 value) { (void)value; }

/* -------------------------------------------------------------------------- */
/* VI                                                                         */
/* -------------------------------------------------------------------------- */
void VIWaitForRetrace(void) {}
void VIConfigurePan(u16 x, u16 y, u16 w, u16 h) { (void)x;(void)y;(void)w;(void)h; }
void VIConfigure(GXRenderModeObj* rm) { (void)rm; }
void VISetBlack(BOOL black) { (void)black; }
void VISetNextFrameBuffer(void* fb) { (void)fb; }
u32 VIGetRetraceCount(void) { return 0; }
u32 VIGetDTVStatus(void) { return 0; }

/* -------------------------------------------------------------------------- */
/* DVD (extra)                                                                */
/* -------------------------------------------------------------------------- */
DVDDiskID* DVDGetCurrentDiskID(void) { return NULL; }
s32 DVDReadPrio(DVDFileInfo* fileInfo, void* addr, s32 length, s32 offset, s32 prio)
{
    (void)fileInfo; (void)addr; (void)length; (void)offset; (void)prio;
    return -1;
}
BOOL DVDFastOpen(s32 entryNum, DVDFileInfo* fileInfo)
{
    (void)entryNum; (void)fileInfo;
    return FALSE;
}
s32 DVDGetCommandBlockStatus(const DVDCommandBlock* block) { (void)block; return 0; }
BOOL DVDReadAsyncPrio(DVDFileInfo* fileInfo, void* addr, s32 length, s32 offset,
    DVDCallback callback, s32 prio)
{
    (void)fileInfo; (void)addr; (void)length; (void)offset; (void)callback; (void)prio;
    return FALSE;
}

/* -------------------------------------------------------------------------- */
/* AR (ARAM)                                                                  */
/* -------------------------------------------------------------------------- */
u32 ARInit(u32* stack_index_addr, u32 num_entries)
{
    (void)stack_index_addr; (void)num_entries;
    return 0;
}
void ARQInit(void) {}
u32 ARGetSize(void) { return 0; }
u32 ARAlloc(u32 length) { (void)length; return 0; }
void ARQPostRequest(ARQRequest* task, u32 owner, u32 type, u32 priority,
    u32 source, u32 dest, u32 length, ARQCallback callback)
{
    (void)task; (void)owner; (void)type; (void)priority;
    (void)source; (void)dest; (void)length; (void)callback;
}

/* -------------------------------------------------------------------------- */
/* PAD                                                                        */
/* -------------------------------------------------------------------------- */
BOOL PADInit(void) { return TRUE; }
u32 PADRead(PADStatus* status) { if (status) memset(status, 0, sizeof(PADStatus)); return 0; }
void PADClamp(PADStatus* status) { (void)status; }
void PADSetSpec(u32 spec) { (void)spec; }
void PADSetAnalogMode(u32 mode) { (void)mode; }
BOOL PADReset(u32 mask) { (void)mask; return TRUE; }
void PADControlMotor(s32 chan, u32 command) { (void)chan; (void)command; }
void PADControlAllMotors(const u32* commandArray) { (void)commandArray; }
BOOL PADRecalibrate(u32 mask) { (void)mask; return TRUE; }

/* -------------------------------------------------------------------------- */
/* EXI / CARD                                                                 */
/* -------------------------------------------------------------------------- */
BOOL EXIUnlock(s32 channel) { (void)channel; return TRUE; }
BOOL EXISelect(s32 channel, u32 device, u32 frequency)
{
    (void)channel;
    (void)device;
    (void)frequency;
    return FALSE;
}
BOOL EXIDeselect(s32 channel) { (void)channel; return TRUE; }
BOOL EXIImmEx(s32 channel, void* buffer, s32 length, u32 type)
{
    (void)channel;
    (void)buffer;
    (void)length;
    (void)type;
    return FALSE;
}
BOOL EXIProbe(s32 channel) { (void)channel; return FALSE; }
s32 CARDGetStatus(s32 chan, s32 fileNo, CARDStat* stat)
{
    (void)chan;
    (void)fileNo;
    if (stat) {
        memset(stat, 0, sizeof(*stat));
    }
    return CARD_RESULT_NOCARD;
}
s32 CARDUnmount(s32 chan)
{
    (void)chan;
    return CARD_RESULT_READY;
}
s32 CARDFastOpen(s32 chan, s32 fileNo, CARDFileInfo* fileInfo)
{
    (void)chan;
    (void)fileNo;
    if (fileInfo) {
        memset(fileInfo, 0, sizeof(*fileInfo));
    }
    return CARD_RESULT_NOFILE;
}
s32 CARDRead(CARDFileInfo* fileInfo, void* addr, s32 length, s32 offset)
{
    (void)fileInfo;
    (void)offset;
    if (addr && length > 0) {
        memset(addr, 0, (size_t)length);
    }
    return CARD_RESULT_NOCARD;
}
s32 CARDClose(CARDFileInfo* fileInfo)
{
    (void)fileInfo;
    return CARD_RESULT_READY;
}
s32 CARDFreeBlocks(s32 chan, s32* byteNotUsed, s32* filesNotUsed)
{
    (void)chan;
    if (byteNotUsed) {
        *byteNotUsed = 0;
    }
    if (filesNotUsed) {
        *filesNotUsed = 0;
    }
    return CARD_RESULT_NOCARD;
}
s32 CARDOpen(s32 chan, const char* fileName, CARDFileInfo* fileInfo)
{
    (void)chan;
    (void)fileName;
    if (fileInfo) {
        memset(fileInfo, 0, sizeof(*fileInfo));
    }
    return CARD_RESULT_NOFILE;
}
s32 CARDDelete(s32 chan, const char* fileName)
{
    (void)chan;
    (void)fileName;
    return CARD_RESULT_NOFILE;
}
s32 CARDRename(s32 chan, const char* oldName, const char* newName)
{
    (void)chan;
    (void)oldName;
    (void)newName;
    return CARD_RESULT_NOFILE;
}
s32 CARDCreate(s32 chan, const char* fileName, u32 size, CARDFileInfo* fileInfo)
{
    (void)chan;
    (void)fileName;
    (void)size;
    if (fileInfo) {
        memset(fileInfo, 0, sizeof(*fileInfo));
    }
    return CARD_RESULT_NOCARD;
}
s32 CARDGetAttributes(s32 chan, s32 fileNo, u8* attr)
{
    (void)chan;
    (void)fileNo;
    if (attr) {
        *attr = 0;
    }
    return CARD_RESULT_NOCARD;
}
s32 CARDSetAttributes(s32 chan, s32 fileNo, u8 attr)
{
    (void)chan;
    (void)fileNo;
    (void)attr;
    return CARD_RESULT_NOCARD;
}
s32 CARDWrite(CARDFileInfo* fileInfo, const void* addr, s32 length, s32 offset)
{
    (void)fileInfo;
    (void)addr;
    (void)length;
    (void)offset;
    return CARD_RESULT_NOCARD;
}
s32 CARDSetStatus(s32 chan, s32 fileNo, CARDStat* stat)
{
    (void)chan;
    (void)fileNo;
    (void)stat;
    return CARD_RESULT_NOCARD;
}
s32 CARDProbeEx(s32 chan, s32* memSize, s32* sectorSize)
{
    (void)chan;
    if (memSize) {
        *memSize = 0;
    }
    if (sectorSize) {
        *sectorSize = 0;
    }
    return CARD_RESULT_NOCARD;
}
s32 CARDMount(s32 chan, void* workArea, CARDCallback detachCallback)
{
    (void)chan;
    (void)workArea;
    (void)detachCallback;
    return CARD_RESULT_NOCARD;
}
s32 CARDCheck(s32 chan)
{
    (void)chan;
    return CARD_RESULT_NOCARD;
}
void CARDInit(void) {}
s32 CARDDeleteAsync(s32 chan, const char* fileName, CARDCallback callback)
{
    (void)chan;
    (void)fileName;
    if (callback) {
        callback(chan, CARD_RESULT_NOFILE);
    }
    return CARD_RESULT_NOFILE;
}
s32 CARDGetResultCode(s32 chan)
{
    (void)chan;
    return CARD_RESULT_NOCARD;
}
s32 CARDMountAsync(s32 chan, void* workArea, CARDCallback detachCallback, CARDCallback attachCallback)
{
    (void)workArea;
    (void)detachCallback;
    if (attachCallback) {
        attachCallback(chan, CARD_RESULT_NOCARD);
    }
    return CARD_RESULT_NOCARD;
}
s32 CARDFormatAsync(s32 chan, CARDCallback callback)
{
    if (callback) {
        callback(chan, CARD_RESULT_NOCARD);
    }
    return CARD_RESULT_NOCARD;
}
s32 CARDCheckAsync(s32 chan, CARDCallback callback)
{
    if (callback) {
        callback(chan, CARD_RESULT_NOCARD);
    }
    return CARD_RESULT_NOCARD;
}
s32 CARDReadAsync(CARDFileInfo* fileInfo, void* addr, s32 length, s32 offset, CARDCallback callback)
{
    (void)fileInfo;
    (void)offset;
    if (addr && length > 0) {
        memset(addr, 0, (size_t)length);
    }
    if (callback) {
        callback(0, CARD_RESULT_NOCARD);
    }
    return CARD_RESULT_NOCARD;
}
s32 CARDWriteAsync(CARDFileInfo* fileInfo, const void* addr, s32 length, s32 offset, CARDCallback callback)
{
    (void)fileInfo;
    (void)addr;
    (void)length;
    (void)offset;
    if (callback) {
        callback(0, CARD_RESULT_NOCARD);
    }
    return CARD_RESULT_NOCARD;
}
s32 CARDCreateAsync(s32 chan, const char* fileName, u32 size, CARDFileInfo* fileInfo, CARDCallback callback)
{
    (void)fileName;
    (void)size;
    if (fileInfo) {
        memset(fileInfo, 0, sizeof(*fileInfo));
    }
    if (callback) {
        callback(chan, CARD_RESULT_NOCARD);
    }
    return CARD_RESULT_NOCARD;
}
s32 __CARDGetStatusEx(s32 chan, s32 fileNo, CARDDir* dirent)
{
    (void)chan;
    (void)fileNo;
    if (dirent) {
        memset(dirent, 0, sizeof(*dirent));
    }
    return CARD_RESULT_NOCARD;
}
s32 __CARDSetStatusEx(s32 chan, s32 fileNo, CARDDir* dirent)
{
    (void)chan;
    (void)fileNo;
    (void)dirent;
    return CARD_RESULT_NOCARD;
}
s32 CARDSetStatusAsync(s32 chan, s32 fileNo, CARDStat* stat, CARDCallback callback)
{
    (void)fileNo;
    (void)stat;
    if (callback) {
        callback(chan, CARD_RESULT_NOCARD);
    }
    return CARD_RESULT_NOCARD;
}
void __CARDReadStatus(s32 chan, CARDCallback callback)
{
    if (callback) {
        callback(chan, CARD_RESULT_NOCARD);
    }
}
void __CARDMountCallback(s32 chan, s32 result)
{
    (void)chan;
    (void)result;
}

/* -------------------------------------------------------------------------- */
/* GBA                                                                        */
/* -------------------------------------------------------------------------- */
void __GBAX01(s32 chan, s32 ret)
{
    (void)chan;
    (void)ret;
}
s32 GBAGetStatus(s32 chan, u8* status)
{
    (void)chan;
    if (status) {
        *status = 0;
    }
    return GBA_NOT_READY;
}
s32 GBAGetProcessStatus(s32 chan, u8* percentp)
{
    (void)chan;
    if (percentp) {
        *percentp = 0;
    }
    return GBA_NOT_READY;
}
s32 GBAJoyBootAsync(s32 chan, s32 palette_color, s32 palette_speed, u8* programp, s32 length, u8* status, GBACallback callback)
{
    (void)chan;
    (void)palette_color;
    (void)palette_speed;
    (void)programp;
    (void)length;
    if (status) {
        *status = 0;
    }
    if (callback) {
        callback(chan, GBA_NOT_READY);
    }
    return GBA_NOT_READY;
}
s32 GBAWrite(s32 chan, u8* src, u8* status)
{
    (void)chan;
    (void)src;
    if (status) {
        *status = 0;
    }
    return GBA_NOT_READY;
}
s32 GBAReset(s32 chan, u8* status)
{
    (void)chan;
    if (status) {
        *status = 0;
    }
    return GBA_NOT_READY;
}
s32 GBARead(s32 chan, u8* dst, u8* status)
{
    (void)chan;
    if (dst) {
        memset(dst, 0, 4);
    }
    if (status) {
        *status = 0;
    }
    return GBA_NOT_READY;
}

/* -------------------------------------------------------------------------- */
/* AI (audio interface)                                                        */
/* -------------------------------------------------------------------------- */
void AIInit(u8* stack)
{
    (void)stack;
}

void AIInitDMA(u32 start_addr, u32 length)
{
    (void)start_addr;
    (void)length;
}

u32 AIGetStreamSampleCount(void)
{
    return 0;
}

void AIResetStreamSampleCount(void) {}

void AISetStreamTrigger(u32 trigger)
{
    (void)trigger;
}

u32 AIGetStreamTrigger(void)
{
    return 0;
}

void AISetStreamPlayState(u32 state)
{
    (void)state;
}

u32 AIGetStreamPlayState(void)
{
    return 0;
}

void AISetStreamSampleRate(u32 rate)
{
    (void)rate;
}

void AISetStreamVolLeft(u8 vol)
{
    (void)vol;
}

void AISetStreamVolRight(u8 vol)
{
    (void)vol;
}

/* -------------------------------------------------------------------------- */
/* DVD (streaming / file) — stubs return failure so stream path gives up      */
/* -------------------------------------------------------------------------- */
BOOL DVDOpen(const char* filename, DVDFileInfo* fileInfo)
{
    (void)filename;
    (void)fileInfo;
    return FALSE;
}

BOOL DVDClose(DVDFileInfo* fileInfo)
{
    (void)fileInfo;
    return TRUE;
}

BOOL DVDPrepareStreamAsync(DVDFileInfo* fileInfo, u32 length, u32 offset,
                           DVDCallback callback)
{
    (void)fileInfo;
    (void)length;
    (void)offset;
    (void)callback;
    return FALSE;
}

s32 DVDGetDriveStatus(void)
{
    return 0; /* e.g. DVD_STATE_END */
}

s32 DVDCancelStream(DVDCommandBlock* block)
{
    (void)block;
    return 0;
}

/* -------------------------------------------------------------------------- */
/* Misc game/system hooks                                                      */
/* -------------------------------------------------------------------------- */
u32 OSGetSoundMode(void) { return 0; }
OSTime OSCalendarTimeToTicks(OSCalendarTime* td)
{
    (void)td;
    return 0;
}
void OSTicksToCalendarTime(OSTime ticks, OSCalendarTime* td)
{
    (void)ticks;
    if (td) {
        memset(td, 0, sizeof(*td));
    }
}
void GBAInit(void) {}
int __strip(void) { return 0; }

/* -------------------------------------------------------------------------- */
/* Probe (profiling) — no-op                                                  */
/* -------------------------------------------------------------------------- */
// void Probe_Start(s32 id, const char* label)
// {
//     (void)id;
//     (void)label;
// }

// void Probe_Finish(s32 id)
// {
//     (void)id;
// }

#endif /* TARGET_PC */
