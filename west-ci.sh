#!/bin/bash

# setup our build environment
echo "$PWD"
ls -al bacnet
west --version
west init -l --mf bacnet/west-ci.yml .
west update > /dev/null 2>&1
