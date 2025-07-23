#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#if defined(__clang__)
# define COMPILER_CLANG 1
#elif defined(_MSC_VER)
# define COMPILER_MSVC 1
#elif defined(__GNUC__)
# define COMPILER_GCC 1
#else
# error Compiler is not supported.
#endif

#if !defined(BUILD_DEBUG)
# define BUILD_DEBUG 1
#endif

#if !defined(COMPILER_CLANG)
# define COMPILER_CLANG 0
#endif
#if !defined(COMPILER_MSVC)
# define COMPILER_MSVC 0
#endif
#if !defined(COMPILER_GCC)
# define COMPILER_GCC 0
#endif

#if COMPILER_MSVC
# define trap() __debugbreak()
#elif COMPILER_CLANG || COMPILER_GCC
# define trap() __builtin_trap()
#else
# error Unknown trap intrinsic for this compiler.
#endif

#define assert_always(x) do {if (!(x)) {trap();}} while(0)
#if BUILD_DEBUG
# define assert(x) assert_always(x)
#else
# define assert(x) (void)(x)
#endif

#define Min(a, b) ((a)<(b)?(a):(b))
#define Max(a, b) ((a)>(b)?(a):(b))

#define internal static
#define global   static
#define local    static

#include <stdint.h>
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;
typedef s8       b8;
typedef s16      b16;
typedef s32      b32;
typedef s64      b64;
typedef float    f32;
typedef double   f64;

typedef struct {
    size_t length;
    size_t capacity;
    s8 array[0];
} Array_Header;

#define array__header(a) ((Array_Header *)((s8 *)(a) - offsetof(Array_Header, array)))
#define array__fits(a, n) ((array_length(a) + (n)) <= array_capacity(a))
#define array__fit(a, n) (array__fits(a, n) ? 0 : ((a) = array__grow((a), array_length(a) + (n), sizeof(*(a)))))

#define array_length(a)   ((a) ? array__header(a)->length : 0)
#define array_capacity(a) ((a) ? array__header(a)->capacity : 0)
#define array_push(a, d)  (array__fit(a, 1), (a)[array_length(a)] = (d), array__header(a)->length++)
#define array_free(a) ((a) ? free(array__header(a)) : (void)0)

internal void *
array__grow(const void *array, size_t length, size_t element_size) {
    size_t capacity = Max(1 + 2*array_capacity(array), length);
    assert(length <= capacity);
    size_t size = offsetof(Array_Header, array) + capacity*element_size;
    Array_Header *header;
    if (array) {
        header = realloc(array__header(array), size);
    } else {
        header = malloc(size);
        header->length = 0;
    }
    header->capacity = capacity;
    return(header->array);
}

internal void
array_test(void) {
    s32 *asdf = NULL;
    enum { N = 1024 };
    for (s32 index = 0;
         index < N;
         ++index) {
        array_push(asdf, index);
    }
    assert(array_length(asdf) == N);
    for (s32 array_index = 0;
         array_index < array_length(asdf);
         ++array_index) {
        assert(asdf[array_index] == array_index);
    }
    array_free(asdf);
}

int
main(void) {
    array_test();
    return(0);
}
