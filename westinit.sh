#!/bin/bash

# projects are expecting a 'bacnet' folder here
ln -s "$PWD" "${PWD%/*}/bacnet"
cd -P .
# setup our build environment
echo $PWD
ls -al
west init -l .
west update > /dev/null 2>&1
