#! /usr/bin/env bash

rm -rf test-reports/

python3 build.py cpu_cfg make gcc/Makefile --CC=~/toolchain/arm-none-eabi/bin/arm-none-eabi-g++ --name=G++ $@
python3 build.py cpu_cfg make gcc/Makefile --CC=~/toolchain/arm-none-eabi/bin/arm-none-eabi-gcc --name=GCC $@
