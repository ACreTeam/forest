#ifndef _JSYSTEM_JUT_BYTESWAP_H
#define _JSYSTEM_JUT_BYTESWAP_H

#include <dolphin/types.h>
#include <bit>

template <class T> struct BE {
    T inner;
    operator T() const {
        return std::byteswap(inner);
    }
};

#endif // _JSYSTEM_JUT_BYTESWAP_H
