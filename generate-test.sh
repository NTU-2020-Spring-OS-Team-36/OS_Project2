#!/usr/bin/env bash

for sz in 1 16 256 4096 65536; do
	mkdir -p "input/${sz}B"
	dd if=/dev/urandom of="input/${sz}B/file1" bs=1 count="${sz}"
	dd if=/dev/urandom of="input/${sz}B/file2" bs=1 count="${sz}"
	dd if=/dev/urandom of="input/${sz}B/file3" bs=1 count="${sz}"
	dd if=/dev/urandom of="input/${sz}B/file4" bs=1 count="${sz}"
	dd if=/dev/urandom of="input/${sz}B/file5" bs=1 count="${sz}"
	dd if=/dev/urandom of="input/${sz}B/file6" bs=1 count="${sz}"
	dd if=/dev/urandom of="input/${sz}B/file7" bs=1 count="${sz}"
	dd if=/dev/urandom of="input/${sz}B/file8" bs=1 count="${sz}"
done
for sz in 1 16 256 4096; do
	mkdir -p "input/${sz}M"
	dd if=/dev/urandom of="input/${sz}M/file" bs=1M count="${sz}"
done
