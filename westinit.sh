#!/bin/bash

# projects are expecting a 'bacnet' folder here
ln -s "$PWD" "${PWD%/*}/bacnet"
cd -P .
# setup our build environment
pip3 install --upgrade west>=1.2.0
west init -l .
west update > /dev/null 2>&1
west zephyr-export
