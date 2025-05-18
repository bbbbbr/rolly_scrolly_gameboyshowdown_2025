#ifndef _COMMON_H
#define _COMMON_H

#include <stdint.h>
#include <stdbool.h>

#define ARRAY_LEN(A)  sizeof(A) / sizeof(A[0])

// GB Sound macros
#define AUDTERM_ALL_LEFT  (AUDTERM_4_LEFT | AUDTERM_3_LEFT | AUDTERM_2_LEFT | AUDTERM_1_LEFT)
#define AUDTERM_ALL_RIGHT (AUDTERM_4_RIGHT | AUDTERM_3_RIGHT | AUDTERM_2_RIGHT | AUDTERM_1_RIGHT)


typedef enum mainstates {
    STATE_PLACEHOLDER = 0
};

#endif // _COMMON_H


