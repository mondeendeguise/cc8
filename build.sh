#!/usr/bin/env bash

set -xe

cc -Wall -Wextra -pedantic -ggdb main.c cc8.c cc8_ops.c -o cc8 -lSDL3
