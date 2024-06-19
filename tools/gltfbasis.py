#!/usr/bin/python3

import argparse
import matplotlib.pyplot as plt
import os
import os.path
import re
import subprocess

argp = argparse.ArgumentParser()
argp.add_argument('--basisu', type=str, required=True)
argp.add_argument('files', type=str, nargs='+')
args = argp.parse_args()

def compress(path, flags):
    temp_path = "/dev/null" if os.name == "posix" else "NUL"
    output = subprocess.check_output([args.basisu, "-file", path, "-output_file", temp_path, "-stats", "-ktx2"] + flags)
    for line in output.splitlines():
        if m := re.match(r".*source image.*?(\d+)x(\d+)", line.decode()):
            pixels = int(m.group(1)) * int(m.group(2))
        elif m := re.match(r".*Compression succeeded.*size (\d+) bytes", line.decode()):
            bytes = int(m.group(1))
        elif m := re.match(r"\.basis RGB Avg:.*RMS: (\d+\.\d+) PSNR: (\d+\.\d+)", line.decode()):
            rms = float(m.group(1))
            psnr = float(m.group(2))

    return {'path': path, 'bpp': bytes * 8 / pixels, 'rms': rms, 'psnr': psnr}

def uastc_stats(path):
    results = []
    for rdo_l in range(0, 20):
        flags = ["-uastc", "-uastc_level", "1", "-uastc_rdo_l", str(rdo_l / 5), "-uastc_rdo_d", "1024"]
        res = compress(path, flags)
        res['rdo_l'] = rdo_l / 5
        results.append(res)
    return results

plt.figure(figsize=(15, 5))

for path in args.files:
    print('Processing', path)
    results = uastc_stats(path)
    etcbase = compress(path, ["-q", "192"])
    name = os.path.basename(path)
    plt.subplot(1, 3, 1)
    line, = plt.plot([r['rdo_l'] for r in results], [r['bpp'] for r in results], label=name)
    plt.axhline(etcbase['bpp'], color=line.get_color(), linestyle='dotted')
    plt.subplot(1, 3, 2)
    line, = plt.plot([r['rdo_l'] for r in results], [r['rms'] for r in results], label=name)
    plt.axhline(etcbase['rms'], color=line.get_color(), linestyle='dotted')
    plt.subplot(1, 3, 3)
    line, = plt.plot([r['rdo_l'] for r in results], [r['psnr'] for r in results], label=name)
    plt.axhline(etcbase['psnr'], color=line.get_color(), linestyle='dotted')

plt.subplot(1, 3, 1)
plt.title('bpp')
plt.legend()
plt.subplot(1, 3, 2)
plt.title('rms')
plt.legend()
plt.subplot(1, 3, 3)
plt.title('psnr')
plt.legend()

plt.savefig('basisu.png')
