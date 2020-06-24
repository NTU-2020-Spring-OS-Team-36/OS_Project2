#!/usr/bin/env bash

for sz in 1 16 256 4096 65536; do
	mkdir -p "input/${sz}k"
	dd if=/dev/urandom of="input/${sz}k/file" count="${sz}"
done
for sz in 1 16 256 4096; do
	mkdir -p "input/${sz}M"
	dd if=/dev/urandom of="input/${sz}M/file" bs=1M count="${sz}"
done
