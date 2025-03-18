#!/bin/bash
./build.sh 2>&1 | sed -E 's/\x1B\[([0-9]{1,3}(;[0-9]{1,3})*)?[mGK]//g'
