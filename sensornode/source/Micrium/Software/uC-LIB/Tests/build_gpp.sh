#! /usr/bin/env bash

#echo "get latest EclipseManager ..."
#curl --user $1 https://bamboo.micrium.com/browse/TECL-EMB-23/artifact/shared/EclipseManager.jar/EclipseManager.jar > EclipseManager.jar
#RETVAL=$?
#[ $RETVAL -ne 0 ] && echo Failure && exit
#[ $RETVAL -eq 0 ] && echo Success

python3 tests.py build.json make Reports/ makefile-cpp $@
RETVAL=$?
[ $RETVAL -ne 0 ] && echo Failure && exit
[ $RETVAL -eq 0 ] && echo Success
echo "Test Complete"
