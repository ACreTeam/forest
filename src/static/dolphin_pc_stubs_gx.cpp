/**
 * PC stubs for Dolphin GX and MTX APIs. Uses exact types from headers.
 */
#ifdef TARGET_PC

#include <dolphin/types.h>
#include <dolphin/dvd.h>
#include <dolphin/os.h>
#include <fstream>
#include <filesystem>

#include <stdint.h>

#if !defined(_WIN32) && !defined(_WIN64)
#  include <sys/stat.h>
#  include <errno.h>
#  include <string.h>
#endif

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#include <limits.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <unistd.h>
#include <limits.h>
#endif

/**
 * Get a unique, stable identifier for the file at the given path.
 * Same path always yields the same id on the same volume until the file is deleted/recreated.
 * Returns 0 on error (file not found, no permission, etc.).
 */
int32_t file_unique_id(const char* path) {
#if defined(_WIN32) || defined(_WIN64)
    HANDLE h = CreateFileA(path,
        FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL);
    if (h == INVALID_HANDLE_VALUE)
        return -1;
    BY_HANDLE_FILE_INFORMATION info;
    if (!GetFileInformationByHandle(h, &info)) {
        CloseHandle(h);
        return -1;
    }
    CloseHandle(h);
    return ((int32_t)info.nFileIndexLow);
#else
    struct stat st;
    if (stat(path, &st) != 0)
        return -1;
    return (int32_t)st.st_ino;
#endif
}

std::filesystem::path getExecutablePath() {
#if defined(_WIN32)
    // Windows implementation
    wchar_t path[MAX_PATH] = { 0 };
    GetModuleFileNameW(NULL, path, MAX_PATH);
    return std::filesystem::path(path);

#elif defined(__linux__)
    // Linux implementation
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    return std::filesystem::path(std::string(result, (count > 0) ? count : 0));

#elif defined(__APPLE__)
    // macOS implementation
    char path[PATH_MAX];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        return std::filesystem::path(path);
    }
    return std::filesystem::path();
#else
    #error "OS not supported"
#endif
}

/**
 * Recursive helper: search under dir for a file with the given id.
 * Returns true and writes the path into out_path (up to out_size) if found.
 */
static bool find_file_by_id_under(int32_t id, const char* dir, char* out_path, size_t out_size) {
#if defined(_WIN32) || defined(_WIN64)
    char pattern[MAX_PATH];
    snprintf(pattern, sizeof(pattern), "%s\\*", dir);
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE)
        return false;
    do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
            continue;
        char full[MAX_PATH];
        snprintf(full, sizeof(full), "%s\\%s", dir, fd.cFileName);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (find_file_by_id_under(id, full, out_path, out_size))
                goto found;
        } else {
            if (file_unique_id(full) == id) {
                strncpy(out_path, full, out_size - 1);
                out_path[out_size - 1] = '\0';
                goto found;
            }
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    return false;
found:
    FindClose(h);
    return true;
#else
    DIR* d = opendir(dir);
    if (!d)
        return false;
    struct dirent* ent;
    bool found = false;
    while ((ent = readdir(d)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;
        char full[4096];
        snprintf(full, sizeof(full), "%s/%s", dir, ent->d_name);
        struct stat st;
        if (lstat(full, &st) != 0)
            continue;
        if (S_ISDIR(st.st_mode)) {
            if (find_file_by_id_under(id, full, out_path, out_size)) {
                found = true;
                break;
            }
        } else if (S_ISREG(st.st_mode)) {
            if (file_unique_id(full) == id) {
                strncpy(out_path, full, out_size - 1);
                out_path[out_size - 1] = '\0';
                found = true;
                break;
            }
        }
    }
    closedir(d);
    return found;
#endif
}

bool file_path_from_id(int32_t id, const char* root_dir, char* out_path, size_t out_size) {
    if (out_path == NULL || out_size == 0)
        return false;
    out_path[0] = '\0';
    return find_file_by_id_under(id, root_dir, out_path, out_size);
}

static int32_t compute_entrynum(const char* filename) {
    std::filesystem::path exeDir = getExecutablePath().parent_path();

    const char* relativePath = filename;
    if (relativePath[0] == '/' || relativePath[0] == '\\') {
        relativePath++; // Move the pointer forward by one character
    }

    std::filesystem::path fullPath = exeDir / "data" / relativePath;
    return file_unique_id(fullPath.string().c_str());
}

static std::ifstream openFile(const char* filename) {
    std::filesystem::path exeDir = getExecutablePath().parent_path();

    const char* relativePath = filename;
    if (relativePath[0] == '/' || relativePath[0] == '\\') {
        relativePath++; // Move the pointer forward by one character
    }

    std::filesystem::path fullPath = exeDir / "data" / relativePath;
    OSReport("Warning: openFile: attempting to open file '%s'.\n", fullPath.string().c_str());
    return std::ifstream(fullPath);
}

s32 DVDConvertPathToEntrynum(const char* path) {
    s32 entrynum = compute_entrynum(path);
    OSReport("Warning: DVDConvertPathToEntrynum(): file '%s' has entrynum '%d'.\n", path, entrynum);
    return entrynum;
}

static s32 DVDOpen(const char* filename, DVDFileInfo* fileInfo) {
    std::ifstream file = openFile(filename);
    
    if (file.is_open()) {
        fileInfo->startAddr = 0;
        
        file.seekg(0, std::ios::end);
        fileInfo->length = file.tellg();
        fileInfo->callback = nullptr;
        fileInfo->cb.state = DVD_STATE_END;
        fileInfo->entrynum = compute_entrynum(filename);
        file.close();

        OSReport("Warning: DVDOpen(): opened file (entrynum %d) '%s'.\n", fileInfo->entrynum, filename);
        return TRUE;
    }

    OSReport("Warning: DVDOpen(): file '%s' was not found.\n", filename);
    return FALSE;
}

BOOL DVDFastOpen(s32 entryNum, DVDFileInfo* fileInfo) {
    // again, fuck it, we ball

    // search through entire data folder for the file
    std::filesystem::path exeDir = getExecutablePath().parent_path();
    std::filesystem::path fullPath = exeDir / "data";

    OSReport("Warning: DVDFastOpen(): searching for file with entrynum '%d' in '%s'.\n", entryNum, fullPath.string().c_str());
    
    for (const auto& entry : std::filesystem::directory_iterator(fullPath)) {
        s32 id = file_unique_id(entry.path().string().c_str());
        // OSReport("Warning: DVDFastOpen(): file '%s' has entrynum '%d'.\n", entry.path().string().c_str(), id);
        if (id == entryNum) {
            return DVDOpen(entry.path().string().c_str(), fileInfo);
        }
    }

    return FALSE;
}

static BOOL DVDClose(DVDFileInfo* fileInfo) {
    return TRUE;
}

static s32 DVDGetCommandBlockStatus(const DVDCommandBlock* block) {
    s32 retVal;

	if (block->state == DVD_STATE_COVER_CLOSED) {
		retVal = DVD_STATE_BUSY;
	} else {
		retVal = block->state;
	}

	return retVal;
}

static BOOL DVDReadAsyncPrio(DVDFileInfo* fileInfo, void* addr, s32 length, s32 offset,
        DVDCallback callback, s32 prio) {
    char path[MAX_PATH];
    BOOL ret = TRUE;

    if (!((0 <= offset) && (offset < fileInfo->length))) {
        OSPanic(__FILE__, 0x2E3, "DVDReadAsync(): specified area is out of the file  ");
    }
    
    if (!((0 <= offset + length) && (offset + length < fileInfo->length + DVD_MIN_TRANSFER_SIZE))) {
        OSPanic(__FILE__, 0x2E9, "DVDReadAsync(): specified area is out of the file  ");
    }

    std::filesystem::path dataDir = getExecutablePath().parent_path() / "data";
    if (!file_path_from_id(fileInfo->entrynum, dataDir.string().c_str(), path, sizeof(path))) {
        OSReport("Warning: DVDReadAsyncPrio(): file (entrynum %d) was not found.\n", fileInfo->entrynum);
        return FALSE;
    }

    fileInfo->callback = callback;
    std::ifstream file(path, std::ios::binary);

    // OSReport("Warning: DVDReadAsyncPrio(): opening file (entrynum %d) '%s'.\n", fileInfo->entrynum, path);

    if (!file.is_open()) {
        OSReport("Warning: DVDReadAsyncPrio(): file '%s' was not found.\n", path);
        return FALSE;
    }

    file.seekg(offset, std::ios::beg);
    file.read(reinterpret_cast<char*>(addr), length);
    
    if (file.fail()) {
        ret = FALSE;
    } else {
        OSReport("DVDReadAsyncPrio(): Read %X bytes from file %s.\n", length, path);
        if (callback) {
            callback(ret, fileInfo);
        }
    }
    
    file.close();
    return ret;
}
#define DVDGetFileInfoStatus(info) DVDGetCommandBlockStatus(&(info)->cb)

#include <dolphin/gx/GXStruct.h>
#include <dolphin/gx/GXFrameBuffer.h>
#include <dolphin/gx/GXEnum.h>
// #include <dolphin/mtx.h>
#include <dolphin/gx/GXTransform.h>
#include <dolphin/gx/GXPixel.h>
#include <dolphin/gx/GXGeometry.h>
#include <dolphin/gx/GXTev.h>
#include <dolphin/gx/GXTexture.h>
#include <dolphin/gx/GXLighting.h>
#include <dolphin/gx/GXCull.h>
#include <dolphin/gx/GXFifo.h>
#include <dolphin/gx/GXBump.h>
#include <dolphin/os/OSThread.h>


typedef struct {
    f32 x, y, z;
} Vec, *VecPtr, Point3d, *Point3dPtr;

typedef f32 Mtx[3][4];
typedef f32 Mtx23[2][3];
typedef f32 Mtx33[3][3];
typedef f32 Mtx44[4][4];
typedef f32 (*MtxPtr)[4];
typedef f32 PSQuaternion[4];

typedef struct Quaternion {
    f32 x, y, z, w;
} Quaternion;

extern "C" {

/* Zero-initialized by C++ rules */
GXRenderModeObj GXNtsc480IntDf;
GXRenderModeObj GXNtsc480Int;

OSThread* GXSetCurrentGXThread(void) { return NULL; }

void GXSetAlphaUpdate(GXBool u) { (void)u; }
void GXSetPixelFmt(GXPixelFmt a, GXZFmt16 b) { (void)a;(void)b; }
void GXSetDither(GXBool d) { (void)d; }
void GXInvalidateTexAll(void) {}
void GXInvalidateVtxCache(void) {}

// void C_MTXOrtho(Mtx44 mtx, f32 t, f32 b, f32 l, f32 r, f32 n, f32 f) { (void)mtx;(void)t;(void)b;(void)l;(void)r;(void)n;(void)f; }

void GXSetProjection(const void* mtx, GXProjectionType type) { (void)mtx;(void)type; }
void GXLoadPosMtxImm(const void* mtx, u32 id) { (void)mtx;(void)id; }
void GXLoadNrmMtxImm(const void* mtx, u32 id) { (void)mtx;(void)id; }
void GXLoadTexMtxImm(const void* mtx, u32 id, GXTexMtxType type) { (void)mtx;(void)id;(void)type; }
void GXSetViewport(f32 a, f32 b, f32 c, f32 d, f32 e, f32 f) { (void)a;(void)b;(void)c;(void)d;(void)e;(void)f; }
void GXSetCurrentMtx(u32 id) { (void)id; }

void GXSetLineWidth(u8 w, GXTexOffset to) { (void)w;(void)to; }
void GXSetPointSize(u8 w, GXTexOffset to) { (void)w;(void)to; }
void GXClearVtxDesc(void) {}
void GXSetNumTexGens(u8 n) { (void)n; }
void GXEnableTexOffsets(GXTexCoordID c, GXBool line, GXBool point) { (void)c;(void)line;(void)point; }

void GXInitTexObj(GXTexObj* o, const void* img, u16 w, u16 h, GXTexFmt fmt, GXTexWrapMode ws, GXTexWrapMode wt, GXBool mip) { (void)o;(void)img;(void)w;(void)h;(void)fmt;(void)ws;(void)wt;(void)mip; }
void GXLoadTexObj(GXTexObj* o, GXTexMapID id) { (void)o;(void)id; }

void GXSetNumChans(u8 n) { (void)n; }
void GXSetChanCtrl(GXChannelID c, GXBool e, GXColorSrc as, GXColorSrc ms, u32 lm, GXDiffuseFn df, GXAttnFn af) { (void)c;(void)e;(void)as;(void)ms;(void)lm;(void)df;(void)af; }
void GXSetChanAmbColor(GXChannelID c, GXColor ac) { (void)c;(void)ac; }
void GXSetChanMatColor(GXChannelID c, GXColor mc) { (void)c;(void)mc; }
void GXSetTexCoordGen2(GXTexCoordID id, GXTexGenType type, GXTexGenSrc src, u32 mtx, GXBool norm, u32 post) { (void)id;(void)type;(void)src;(void)mtx;(void)norm;(void)post; }

void GXSetNumTevStages(u8 n) { (void)n; }
void GXSetTevColorIn(GXTevStageID s, GXTevColorArg a, GXTevColorArg b, GXTevColorArg c, GXTevColorArg d) { (void)s;(void)a;(void)b;(void)c;(void)d; }
void GXSetTevAlphaIn(GXTevStageID s, GXTevAlphaArg a, GXTevAlphaArg b, GXTevAlphaArg c, GXTevAlphaArg d) { (void)s;(void)a;(void)b;(void)c;(void)d; }
void GXSetTevOrder(GXTevStageID s, GXTexCoordID coord, GXTexMapID map, GXChannelID col) { (void)s;(void)coord;(void)map;(void)col; }
void GXSetTevColorOp(GXTevStageID s, GXTevOp op, GXTevBias bias, GXTevScale scale, GXBool clamp, GXTevRegID out) { (void)s;(void)op;(void)bias;(void)scale;(void)clamp;(void)out; }
void GXSetTevAlphaOp(GXTevStageID s, GXTevOp op, GXTevBias bias, GXTevScale scale, GXBool clamp, GXTevRegID out) { (void)s;(void)op;(void)bias;(void)scale;(void)clamp;(void)out; }
void GXSetTevOp(GXTevStageID id, GXTevMode mode) { (void)id;(void)mode; }
void GXSetAlphaCompare(GXCompare c0, u8 r0, GXAlphaOp op, GXCompare c1, u8 r1) { (void)c0;(void)r0;(void)op;(void)c1;(void)r1; }
void GXSetZTexture(GXZTexOp op, GXTexFmt fmt, u32 bias) { (void)op;(void)fmt;(void)bias; }
void GXSetTevKColorSel(GXTevStageID s, GXTevKColorSel sel) { (void)s;(void)sel; }
void GXSetTevKAlphaSel(GXTevStageID s, GXTevKAlphaSel sel) { (void)s;(void)sel; }
void GXSetTevSwapMode(GXTevStageID s, GXTevSwapSel ras, GXTevSwapSel tex) { (void)s;(void)ras;(void)tex; }
void GXSetTevSwapModeTable(GXTevSwapSel t, GXTevColorChan r, GXTevColorChan g, GXTevColorChan b, GXTevColorChan a) { (void)t;(void)r;(void)g;(void)b;(void)a; }
void GXSetTevDirect(GXTevStageID s) { (void)s; }

void GXSetNumIndStages(u8 n) { (void)n; }
void GXSetIndTexCoordScale(GXIndTexStageID s, GXIndTexScale ss, GXIndTexScale st) { (void)s;(void)ss;(void)st; }

void GXSetFog(GXFogType type, f32 start, f32 end, f32 nearz, f32 farz, GXColor color) { (void)type;(void)start;(void)end;(void)nearz;(void)farz;(void)color; }
void GXSetFogRangeAdj(GXBool enable, u16 center, GXFogAdjTable* table) { (void)enable;(void)center;(void)table; }
void GXSetBlendMode(GXBlendMode t, GXBlendFactor sf, GXBlendFactor df, GXLogicOp op) { (void)t;(void)sf;(void)df;(void)op; }
void GXSetColorUpdate(GXBool e) { (void)e; }
void GXSetZMode(GXBool e, GXCompare f, GXBool u) { (void)e;(void)f;(void)u; }
void GXSetZCompLoc(GXBool b) { (void)b; }
void GXSetDstAlpha(GXBool e, u8 a) { (void)e;(void)a; }
void GXSetFieldMask(GXBool odd, GXBool even) { (void)odd;(void)even; }
void GXSetFieldMode(GXBool fm, GXBool half) { (void)fm;(void)half; }

void GXSetCoPlanar(GXBool e) { (void)e; }
void GXSetCullMode(GXCullMode m) { (void)m; }
void GXSetClipMode(GXClipMode m) { (void)m; }
void GXSetScissor(u32 left, u32 top, u32 wd, u32 ht) { (void)left;(void)top;(void)wd;(void)ht; }
void GXSetScissorBoxOffset(s32 x, s32 y) { (void)x;(void)y; }

/* MTX functions are provided by aurora::mtx on PC. */

/* GX Texture (additional) */
void GXInitTexObjCI(GXTexObj* o, const void* img, u16 w, u16 h, GXCITexFmt fmt, GXTexWrapMode ws, GXTexWrapMode wt, GXBool mip, u32 tlut) { (void)o;(void)img;(void)w;(void)h;(void)fmt;(void)ws;(void)wt;(void)mip;(void)tlut; }
void GXInitTexObjLOD(GXTexObj* o, GXTexFilter minF, GXTexFilter magF, f32 minL, f32 maxL, f32 bias, GXBool biasClamp, GXBool edgeLod, GXAnisotropy aniso) { (void)o;(void)minF;(void)magF;(void)minL;(void)maxL;(void)bias;(void)biasClamp;(void)edgeLod;(void)aniso; }
void GXInitTlutObj(GXTlutObj* o, const void* lut, GXTlutFmt fmt, u16 n) { (void)o;(void)lut;(void)fmt;(void)n; }
void GXLoadTlut(const GXTlutObj* o, GXTlut idx) { (void)o;(void)idx; }

/* GX Tev */
void GXSetTevColor(GXTevRegID id, GXColor c) { (void)id;(void)c; }

/* GX Vert (TARGET_PC non-inline) */
void GXPosition3f32(f32 x, f32 y, f32 z) { (void)x;(void)y;(void)z; }
void GXPosition2f32(f32 x, f32 y) { (void)x;(void)y; }
void GXPosition2s16(s16 x, s16 y) { (void)x;(void)y; }
void GXPosition2u16(u16 x, u16 y) { (void)x;(void)y; }
void GXTexCoord2u16(u16 s, u16 t) { (void)s;(void)t; }
void GXNormal3f32(f32 x, f32 y, f32 z) { (void)x;(void)y;(void)z; }
void GXColor1u32(u32 c) { (void)c; }
void GXTexCoord2s16(s16 s, s16 t) { (void)s;(void)t; }
void GXTexCoord2f32(f32 s, f32 t) { (void)s;(void)t; }
void GXTexCoord2u8(u8 s, u8 t) { (void)s;(void)t; }
void GXEnd(void) {}

/* GX Geometry */
void GXSetVtxDesc(GXAttr attr, GXAttrType type) { (void)attr;(void)type; }
void GXSetVtxAttrFmt(GXVtxFmt fmt, GXAttr attr, GXCompCnt cnt, GXCompType type, u8 frac) { (void)fmt;(void)attr;(void)cnt;(void)type;(void)frac; }
void GXBegin(GXPrimitive type, GXVtxFmt fmt, u16 nverts) { (void)type;(void)fmt;(void)nverts; }

/* GX Light */
void GXInitLightAttn(GXLightObj* o, f32 a0, f32 a1, f32 a2, f32 k0, f32 k1, f32 k2) { (void)o;(void)a0;(void)a1;(void)a2;(void)k0;(void)k1;(void)k2; }
void GXInitLightColor(GXLightObj* o, GXColor c) { (void)o;(void)c; }
void GXInitLightDir(GXLightObj* o, f32 x, f32 y, f32 z) { (void)o;(void)x;(void)y;(void)z; }
void GXInitLightPos(GXLightObj* o, f32 x, f32 y, f32 z) { (void)o;(void)x;(void)y;(void)z; }
void GXLoadLightObjImm(GXLightObj* o, GXLightID id) { (void)o;(void)id; }

/* GX Fifo / Misc */
void GXInit(void) {}
void GXInitFifoBase(GXFifoObj* fifo, void* base, u32 size) { (void)fifo;(void)base;(void)size; }
void GXInitFifoPtrs(GXFifoObj* fifo, void* readPtr, void* writePtr) { (void)fifo;(void)readPtr;(void)writePtr; }
void GXSaveCPUFifo(GXFifoObj* fifo) { (void)fifo; }
OSThread* GXGetCurrentGXThread(void) { return NULL; }
void GXGetGPStatus(GXBool* a, GXBool* b, GXBool* c, GXBool* d, GXBool* e) { (void)a;(void)b;(void)c;(void)d;(void)e; }
void GXFlush(void) {}
void GXDrawDone(void) {}
void GXAbortFrame(void) {}
void GXCallDisplayList(const void* list, u32 nbytes) { (void)list;(void)nbytes; }
void GXPixModeSync(void) {}
void GXReadXfRasMetric(u32* a, u32* b, u32* c, u32* d) { (void)a;(void)b;(void)c;(void)d; }
void GXSetDrawDone(void) {}
typedef void (*GXDrawDoneCallback)(void);
void GXSetDrawDoneCallback(GXDrawDoneCallback cb) { (void)cb; }

/* GX FrameBuffer */
void GXSetCopyClear(GXColor c, u32 z) { (void)c;(void)z; }
void GXSetCopyClamp(GXFBClamp c) { (void)c; }
void GXSetCopyFilter(GXBool aa, u8 sample[12][2], GXBool vf, u8* vfilter) { (void)aa;(void)sample;(void)vf;(void)vfilter; }
void GXSetDispCopySrc(u16 left, u16 top, u16 wd, u16 ht) { (void)left;(void)top;(void)wd;(void)ht; }
void GXSetDispCopyDst(u16 wd, u16 ht) { (void)wd;(void)ht; }
void GXSetDispCopyGamma(GXGamma g) { (void)g; }
u32 GXSetDispCopyYScale(f32 s) { (void)s; return 0; }
void GXCopyDisp(void* dest, GXBool clear) { (void)dest;(void)clear; }
void GXSetTexCopySrc(u16 left, u16 top, u16 wd, u16 ht) { (void)left;(void)top;(void)wd;(void)ht; }
void GXSetTexCopyDst(u16 wd, u16 ht, GXTexFmt fmt, GXBool mipmap) { (void)wd;(void)ht;(void)fmt;(void)mipmap; }
void GXCopyTex(void* dest, GXBool clear) { (void)dest;(void)clear; }
void GXSetTexCoordScaleManually(GXTexCoordID coord, GXBool enable, u16 ss, u16 ts)
{
    (void)coord;
    (void)enable;
    (void)ss;
    (void)ts;
}
void GXSetTexCoordBias(GXTexCoordID coord, GXBool s_enable, GXBool t_enable)
{
    (void)coord;
    (void)s_enable;
    (void)t_enable;
}
void GXSetIndTexOrder(GXIndTexStageID ind_stage, GXTexCoordID tex_coord, GXTexMapID tex_map)
{
    (void)ind_stage;
    (void)tex_coord;
    (void)tex_map;
}
void GXSetIndTexMtx(GXIndTexMtxID mtx_sel, const void* offset, s8 scale_exp)
{
    (void)mtx_sel;
    (void)offset;
    (void)scale_exp;
}
void GXSetTevIndirect(GXTevStageID tev_stage, GXIndTexStageID ind_stage, GXIndTexFormat format,
                      GXIndTexBiasSel bias_sel, GXIndTexMtxID mtx_sel, GXIndTexWrap wrap_s,
                      GXIndTexWrap wrap_t, GXBool add_prev, GXBool utc_lod, GXIndTexAlphaSel alpha_sel)
{
    (void)tev_stage;
    (void)ind_stage;
    (void)format;
    (void)bias_sel;
    (void)mtx_sel;
    (void)wrap_s;
    (void)wrap_t;
    (void)add_prev;
    (void)utc_lod;
    (void)alpha_sel;
}

} /* extern "C" */

#endif
