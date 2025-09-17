#!/bin/bash

./run.sh debug &
QEMU_PID=$!

while ! nc -z localhost 1234; do
  sleep 0.1
done