#ifndef JKRDISPOSER_H
#define JKRDISPOSER_H

#include "types.h"
#include "JSystem/JSupport/JSUList.h"

#ifdef __cplusplus
extern "C" {

class JKRHeap;

class JKRDisposer {
  public:
    JKRDisposer();
    virtual ~JKRDisposer();

    void* operator new(size_t, void* ptr) noexcept {
        return ptr;
    }

    void* operator new(size_t);
    void* operator new(size_t, s32);
    void* operator new(size_t, JKRHeap*, int);
    void* operator new[](size_t);
    void* operator new[](size_t, s32);
    void* operator new[](size_t, JKRHeap*, int);
    void operator delete(void*) noexcept;
    void operator delete[](void*) noexcept;

  public:
    JKRHeap* mRootHeap;                 // _4
    JSULink<JKRDisposer> mPointerLinks; // _8
};
}
#endif

#endif
