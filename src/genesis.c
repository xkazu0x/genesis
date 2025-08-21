#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <ctype.h>

////////////////////////////////
// NOTE(xkazu0x): Clang Context Cracking

#if defined(__clang__)
# define COMPILER_CLANG 1

# if defined(_WIN32)
#  define OS_WINDOWS 1
# elif defined(__gnu_linux__) || defined(__linux__)
#  define OS_LINUX 1
# elif defined(__APPLE__) && defined(__MACH__)
#  define OS_MAC 1
# else
#  error Compiler/OS is not supported.
# endif

# if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64)
#  define ARCH_X64 1
# elif defined(i386) || defined(__i386) || defined(__i386__)
#  define ARCH_X86 1
# elif defined(__aarch64__)
#  define ARCH_ARM64 1
# elif defined(__arm__)
#  define ARCH_ARM32 1
# else
#  error Architecture is not supported.
# endif

////////////////////////////////
// NOTE(xkazu0x): MSVC Context Cracking

#elif defined(_MSC_VER)
# define COMPILER_MSVC 1

# if defined(_WIN32)
#  define OS_WINDOWS 1
# else
#  error Compiler/OS is not supported.
# endif

# if defined(_M_AMD64)
#  define ARCH_X64 1
# elif defined(_M_IX86)
#  define ARCH_X86 1
# elif defined(_M_ARM64)
#  define ARCH_ARM64 1
# elif defined(_M_ARM)
#  define ARCH_ARM32 1
# else
#  error Architecture not supported.
# endif

////////////////////////////////
// NOTE(xkazu0x): GCC Context Cracking

#elif defined(__GNUC__)
# define COMPILER_GCC 1

# if defined(_WIN32)
#  define OS_WINDOWS 1
# elif defined(__gnu_linux__)
#  define OS_LINUX 1
# elif defined(__APPLE__) && defined(__MACH__)
#  define OS_MAC 1
# else
#  error Compiler/OS is not supported.
# endif

# if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64)
#  define ARCH_X64 1
# elif defined(i386) || defined(__i386) || defined(__i386__)
#  define ARCH_X86 1
# elif defined(__aarch64__)
#  define ARCH_ARM64 1
# elif defined(__arm__)
#  define ARCH_ARM32 1
# else
#  error Architecture is not supported.
# endif

#else
# error Compiler is not supported.
#endif

////////////////////////////////
// NOTE(xkazu0x): Build Option Cracking

#if !defined(BUILD_DEBUG)
# define BUILD_DEBUG 1
#endif

////////////////////////////////
// NOTE(xkazu0x): Helper Macros

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

#define MIN(a, b) ((a)<(b)?(a):(b))
#define MAX(a, b) ((a)>(b)?(a):(b))

#define internal static
#define global   static
#define local    static

////////////////////////////////
// NOTE(xkazu0x): Safe Functions

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
// NOTE(xkazu0x): Stretchy Buffer

typedef struct {
    size_t length;
    size_t capacity;
    char buffer[0];
} Buffer_Header;

#define buffer__header(b) ((Buffer_Header *)((char *)(b) - offsetof(Buffer_Header, buffer)))
#define buffer__fits(b, n) (buffer_length(b) + (n) <= buffer_capacity(b))
#define buffer__fit(b, n) (buffer__fits((b), (n)) ? 0 : ((b) = buffer__grow((b), buffer_length(b) + (n), sizeof(*(b)))))

#define buffer_length(b) ((b) ? buffer__header(b)->length : 0)
#define buffer_capacity(b) ((b) ? buffer__header(b)->capacity : 0)
#define buffer_push(b, d) (buffer__fit((b), 1), (b)[buffer__header(b)->length++] = (d))
#define buffer_free(b) ((b) ? (free(buffer__header(b)), (b) = 0) : 0)

internal void *
buffer__grow(const void *buffer, size_t length, size_t element_size) {
    size_t capacity = MAX(1 + 2*buffer_capacity(buffer), length);
    assert(length <= capacity);
    size_t size = offsetof(Buffer_Header, buffer) + capacity*element_size;
    Buffer_Header *header;
    if (buffer) {
        header = xrealloc(buffer__header(buffer), size);
    } else {
        header = xmalloc(size);
        header->length = 0;
    }
    header->capacity = capacity;
    return(header->buffer);
}

internal void
buffer_test(void) {
    int *test = 0;
    assert(buffer_length(test) == 0);
    enum { N = 1024 };
    for (int index = 0;
         index < N;
         ++index) {
        buffer_push(test, index);
    }
    assert(buffer_length(test) == N);
    for (int index = 0;
         index < N;
         ++index) {
        assert(test[index] == index);
    }
    buffer_free(test);
    assert(test == 0);
    assert(buffer_length(test) == 0);
}

////////////////////////////////
// NOTE(xkazu0x): Lexing

typedef enum {
    TOKEN_LAST_CHAR = 127,
    TOKEN_INT,
    TOKEN_NAME,
} Token_Kind;

typedef struct {
    Token_Kind kind;
    const char *start;
    const char *end;
    union {
        uint64_t value;
    };
} Token;

global Token global_token;
const char *global_stream;

internal void
next_token() {
    global_token.start = global_stream;
    switch (*global_stream) {
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
            uint64_t value = 0;
            if (isdigit(*global_stream)) {
                value *= 10;
                value += *global_stream++ - '0';
            }
            global_token.kind = TOKEN_INT;
            global_token.value = value;
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
            while (isalnum(*global_stream) || (*global_stream == '_')) {
                global_stream++;
            }
            global_token.kind = TOKEN_NAME;
        } break;
            
        default: {
            global_token.kind = *global_stream++;
        } break;
    }
    global_token.end = global_stream;
}

internal void
print_token(Token token) {
    switch (token.kind) {
        case TOKEN_INT: {
            printf("Token Int: %llu\n", token.value);
        } break;
            
        case TOKEN_NAME: {
            printf("Token Name: %.*s\n", (int)(token.end - token.start), token.start);
        } break;
            
        default: {
            printf("Token '%c'\n", token.kind);
        } break;
    }
};

internal void
lexing_test(void) {
    char *source = "+()_HELLO1,234+FOO!994";
    global_stream = source;
    next_token();
    while (global_token.kind) {
        print_token(global_token);
        next_token();
    }
}

int
main(void) {
    buffer_test();
    lexing_test();
    return(0);
}
