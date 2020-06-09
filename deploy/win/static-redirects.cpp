
#include <stdio.h>

extern "C" {
    void __imp__assert(char const *msg, char const *fn, unsigned line) {}
    void __imp__wassert(wchar_t const *msg, wchar_t const *fn, unsigned line) {}
    void __imp_clearerr(FILE *f) { clearerr(f); }
    void __imp_rewind(FILE *f) { rewind(f); }
}

