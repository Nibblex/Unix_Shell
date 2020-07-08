#ifndef BUILTIN_H
#define BUILTIN_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "bstring/bstrlib.h"
#include "command.h"
#include "tests/syscall_mock.h"

typedef enum {
    BUILTIN_EXIT = 0,
    BUILTIN_CHDIR = 1,
    BUILTIN_UNKNOWN = -1
} cmd_id;

cmd_id builtin_index(const scpipe pipe);

void builtin_run(const scpipe pipe);

#endif