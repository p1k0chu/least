#!/usr/bin/env bash

cc=gcc

make -C lib/cbuild && \
    $cc build.c -Llib/cbuild -lcbuild -Ilib/cbuild/include -o build && \
    touch build.c
