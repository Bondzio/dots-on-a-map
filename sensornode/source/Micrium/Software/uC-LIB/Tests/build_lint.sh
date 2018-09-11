#! /usr/bin/env bash

python tests.py build.json lint Reports/ makefile-lint --log=DEBUG --number=0
RETVAL=$?
[ $RETVAL -ne 0 ] && echo Failure && exit
[ $RETVAL -eq 0 ] && echo Success
echo "Test Complete"