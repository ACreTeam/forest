#include "types.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

void HotResetIplMenu(void) {}
int dvderr_draw(void) { return 0; }
void JW_BeginFrame(void) {}
void JW_EndFrame(void) {}
void JW_Init3(void) {}
void JW_JUTGamePad_read(void) {}
void JW_getPadStatus(void* padStatus) { (void)padStatus; }
void JW_SetFamicomMode(int enabled) { (void)enabled; }
void VIFlush(void) {}
void VIInit(void) {}
void JW_SetLogoMode(int enabled) { (void)enabled; }
int JW_setClearColor(u8 r, u8 g, u8 b)
{
    (void)r;
    (void)g;
    (void)b;
    return 0;
}
u32 JW_GetAramAddress(int res_no)
{
    (void)res_no;
    return 0;
}
u32 JW_GetResSizeFileNo(int res_no)
{
    (void)res_no;
    return 0;
}
u8* _JW_GetResourceAram(u32 aram_addr, u8* dst, u32 size)
{
    (void)aram_addr;
    if (dst && size > 0) {
        memset(dst, 0, size);
    }
    return dst;
}
f32 JW_JUTGamepad_getSubStickValue(void) { return 0.0f; }
s16 JW_JUTGamepad_getSubStickAngle(void) { return 0; }
void JW_SetLowResoMode(int enabled) { (void)enabled; }
void JW_SetProgressiveMode(int enabled) { (void)enabled; }

void GXBeginDisplayList(void* list, u32 size)
{
    (void)list;
    (void)size;
}
u32 GXEndDisplayList(void) { return 0; }

void make_dl_nintendo_logo(void* gpp, u32 alpha)
{
    (void)gpp;
    (void)alpha;
}

void mGcgba_InitVar(void) {}
void mGcgba_Init(void) {}
void mGcgba_EndComm(void) {}
int mGcgba_ConnectEnabled(void) { return 0; }
int mGcgba_Boot(u8* data_buf, size_t buf_size)
{
    (void)data_buf;
    (void)buf_size;
    return 0;
}
int mGcgba_IsEditor(void) { return 0; }
int mGcgba_IsIsland(void) { return 0; }
int mGcgba_Read(u8* read_buf, size_t read_buf_size, u8 prog_type)
{
    (void)prog_type;
    if (read_buf && read_buf_size > 0) {
        memset(read_buf, 0, read_buf_size);
    }
    return 0;
}
int mGcgba_Recv(u8* recv_bufp, size_t recv_buf_size)
{
    if (recv_bufp && recv_buf_size > 0) {
        memset(recv_bufp, 0, recv_buf_size);
    }
    return 0;
}
int mGcgba_CheckRecv(u8* recv_bufp, size_t recv_buf_size)
{
    if (recv_bufp && recv_buf_size > 0) {
        memset(recv_bufp, 0, recv_buf_size);
    }
    return 0;
}
int mGcgba_Send(u8* send_bufp, size_t send_buf_size)
{
    (void)send_bufp;
    (void)send_buf_size;
    return 0;
}
int mGcgba_send_eAppri(u8* eappli_p, size_t eappli_size)
{
    (void)eappli_p;
    (void)eappli_size;
    return 0;
}
void OSSetSoundMode(u32 mode) { (void)mode; }
int OSCheckActiveThreads(void) { return 0; }

void* boot_copyDate = NULL;
char __Creator__[] = "PC";
char __DateTime__[] = "PC";

int _strip(void) { return 0; }
int _abs(int x) { return x < 0 ? -x : x; }
int __abs(int x) { return x < 0 ? -x : x; }
void* __alloca(size_t size) { return malloc(size); }
void* convert_partial_address(void* p) { return p; }

void* gamePad = NULL;
u8 nintendo_hi_0[1];
