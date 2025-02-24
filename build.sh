#!/usr/bin/env bash

set -xe

cc -Wall -Wextra -pedantic -ggdb main.c -o cc8 -lSDL3
