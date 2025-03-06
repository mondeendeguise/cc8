#!/usr/bin/env bash

set -xe

# cc -Wall -Wextra -pedantic -ggdb main.c cc8.c cc8_ops.c -o cc8 -lSDL3
cc -Wall -Wextra -pedantic -O2 -ggdb main.c cc8.c cc8_ops.c -o cc8 -lSDL3
cc -Wall -Wextra -pedantic -ggdb cc8c.c -o cc8c
