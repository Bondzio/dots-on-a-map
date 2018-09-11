#! /usr/bin/env bash

python3 export.py           \
-i ../../cpu_core.c         \
-i ../../ARM-Cortex-A/GNU   \
-I ../../ARM-Cortex-A/GNU   \
-I ../../                   \
-I ../../Cfg/Template       \
-I ../../../uC-LIB          \
Makefile
