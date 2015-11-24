#!/bin/bash
LD_PRELOAD=/usr/local/lib/libjemalloc.so:$LD_PRELOAD

export LD_PRELOAD
