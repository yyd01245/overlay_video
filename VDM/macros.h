
#ifndef _H_MACROS_
#define _H_MACROS_

#include<sys/types.h>




#ifndef __cplusplus

#define false 0
#define true 1
#endif


#define _LIKELY(x) \
    (__builtin_expect(!!(x),1))

#define _UNLIKELY(x) \
    (__builtin_expect(!!(x),0))

#define _ELEMENTSOF(x) \
    (sizeof(x)/sizeof((x)[0]))

#define _MAX(a,b)           \
    __extension__ ({        \
        typeof(a) _a = (a);     \
        typeof(b) _b = (b);     \
        _a > _b ? _a : _b;      \
        })

#define _MIN(a,b)           \
    __extension__ ({        \
        typeof(a) _a = (a);     \
        typeof(b) _b = (b);     \
        _a < _b ? _a : _b;      \
        })


#define _CLAMP(x, low, high)                                  \
    __extension__ ({                                          \
        typeof(x) _x = (x);                                         \
        typeof(low) _low = (low);                                   \
        typeof(high) _high = (high);                                \
        ((_x > _high) ? _high : ((_x < _low) ? _low : _x));         \
        })


#define _ROUND_UP(a, b)             \
    __extension__ ({                \
        typeof(a) _a = (a);         \
        typeof(b) _b = (b);         \
        ((_a + _b - 1) / _b) * _b;  \
        })

#define _ROUND_DOWN(a, b)           \
    __extension__ ({                \
        typeof(a) _a = (a);         \
        typeof(b) _b = (b);         \
        (_a / _b) * _b;             \
        })


/* Rounds down */
static inline void* _ALIGN_PTR(const void *p) {
        return (void*) (((size_t) p) & ~(sizeof(void*) - 1));

}

/* Rounds up */
static inline size_t _ALIGN(size_t l) {
        return ((l + sizeof(void*) - 1) & ~(sizeof(void*) - 1));

}








#endif
