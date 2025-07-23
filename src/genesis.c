#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <ctype.h>

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

internal void *
xrealloc(void *ptr, size_t size) {
    void *result = realloc(ptr, size);
    if (!result) {
        perror("xrealloc failed");
        exit(1);
    }
    return(result);
}

internal void *
xmalloc(size_t size) {
    void *result = malloc(size);
    if (!result) {
        perror("xmalloc failed");
        exit(1);
    }
    return(result);
}

////////////////////////////////
// NOTE(xkazu0x): Runtime Array

typedef struct {
    size_t length;
    size_t capacity;
    s8 array[0];
} Array_Header;

#define array__header(a) ((Array_Header *)((s8 *)(a) - offsetof(Array_Header, array)))
#define array__fits(a, n) ((array_length(a) + (n)) <= array_capacity(a))
#define array__fit(a, n) (array__fits((a), (n)) ? 0 : ((a) = array__grow((a), array_length(a) + (n), sizeof(*(a)))))

#define array_length(a)   ((a) ? array__header(a)->length : 0)
#define array_capacity(a) ((a) ? array__header(a)->capacity : 0)
#define array_push(a, d)  (array__fit((a), 1), (a)[array__header(a)->length++] = (d))
#define array_free(a) ((a) ? (free(array__header(a)), (a) = NULL) : 0)

internal void *
array__grow(const void *array, size_t length, size_t element_size) {
    size_t capacity = Max(1 + 2*array_capacity(array), length);
    assert(length <= capacity);
    size_t size = offsetof(Array_Header, array) + capacity*element_size;
    Array_Header *header;
    if (array) {
        header = xrealloc(array__header(array), size);
    } else {
        header = xmalloc(size);
        header->length = 0;
    }
    header->capacity = capacity;
    return(header->array);
}

internal void
array_test(void) {
    s32 *asdf = NULL;
    assert(array_length(asdf) == 0);
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
    assert(asdf == NULL);
    assert(array_length(asdf) == 0);
}

////////////////////////////////
// NOTE(xkazu0x): Lexing

typedef enum {
    TokenType_LastChar = 127,
    TokenType_Int,
    TokenType_Name,
} Token_Type;

typedef struct {
    Token_Type type;
    const char *start;
    const char *end;
    union {
        u64 value;
    };
} Token;

Token token;
const char *stream;

internal void
next_token(void) {
    token.start = stream;
    switch (*stream) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': {
            u64 value = 0;
            while (isdigit(*stream)) {
                value *= 10;
                value += *stream++ - '0';
            }
            token.type = TokenType_Int;
            token.value = value;
        } break;

        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'g':
        case 'h':
        case 'i':
        case 'j':
        case 'k':
        case 'l':
        case 'm':
        case 'n':
        case 'o':
        case 'p':
        case 'q':
        case 'r':
        case 's':
        case 't':
        case 'u':
        case 'v':
        case 'w':
        case 'x':
        case 'y':
        case 'z':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case 'G':
        case 'H':
        case 'I':
        case 'J':
        case 'K':
        case 'L':
        case 'M':
        case 'N':
        case 'O':
        case 'P':
        case 'Q':
        case 'R':
        case 'S':
        case 'T':
        case 'U':
        case 'V':
        case 'W':
        case 'X':
        case 'Y':
        case 'Z':
        case '_': {
            while (isalnum(*stream) || *stream == '_') {
                stream++;
            }
            token.type = TokenType_Name;
        } break;
            
        default: {
            token.type = *stream++;
        } break;
    }
    token.end = stream;
}

internal void
print_token() {
    switch (token.type) {
        case TokenType_Int: {
            printf("Token Int: %llu\n", token.value);
        } break;
        case TokenType_Name: {
            printf("Token Name: %.*s\n", (s32)(token.end - token.start), token.start);
        } break;
        default: {
            printf("Token '%c'\n", token.type);
        } break;
    }
}

internal void
lex_test(void) {
    char *source = "+()_HELLO1,234+FOO!994";
    stream = source;
    next_token();
    while (token.type) {
        print_token();
        next_token();
    }
}

////////////////////////////////
// NOTE(xkazu0x): main proc

int
main(void) {
    array_test();
    lex_test();
    printf("goodbye world");
    return(0);
}
