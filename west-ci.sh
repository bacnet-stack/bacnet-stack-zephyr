#!/bin/bash

# setup our build environment
west --version
west init -l --mf ./west-ci.yml .
west update > /dev/null 2>&1
