#!/usr/bin/env bash
#

sudo setcap cap_ipc_owner,cap_ipc_lock+ep ./build/producer
./build/producer
