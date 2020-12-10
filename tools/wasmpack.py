#!/usr/bin/python3

import sys

# regenerate with wasmpack.py generate
table = [32, 0, 65, 253, 3, 1, 2, 34, 4, 106, 6, 5, 11, 8, 7, 20, 13, 33, 12, 16, 128, 9, 116, 64, 19, 113, 127, 15, 10, 21, 22, 14, 255, 66, 24, 54, 136, 107, 18, 23, 192, 26, 114, 118, 132, 17, 77, 101, 130, 144, 27, 87, 131, 44, 45, 74, 156, 154, 70, 167]

base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

def encode(buffer):
	result = ''

	for ch in buffer.read():
		if ch in table:
			index = table.index(ch)
			result += base64[index]
		else:
			result += base64[60 + ch // 64]
			result += base64[ch % 64]

	return result

def stats(buffer):
	hist = [0] * 256
	for ch in buffer.read():
		hist[ch] += 1

	result = [i for i in range(256)]
	result.sort(key=lambda i: hist[i], reverse=True)

	return result

if sys.argv[-1] == 'generate':
	print(stats(sys.stdin.buffer)[:60])
else:
	print(encode(sys.stdin.buffer))
