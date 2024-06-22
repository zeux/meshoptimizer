#!/usr/bin/python3

import argparse
import concurrent.futures
import matplotlib.pyplot as plt
import os
import os.path
import re
import subprocess

argp = argparse.ArgumentParser()
argp.add_argument('--basisu', type=str, required=True)
argp.add_argument('--graph', type=str, default="basisu.png")
argp.add_argument('--etcbase', action='store_true')
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

def stats(path):
    with concurrent.futures.ThreadPoolExecutor(16) as executor:
        futures = []
        for i in range(0, 26):
            rdo_l = i / 5
            flags = ["-uastc", "-uastc_level", "1", "-uastc_rdo_l", str(rdo_l), "-uastc_rdo_d", "1024"]
            futures.append((executor.submit(compress, path, flags), rdo_l))
        concurrent.futures.wait([f for (f, r) in futures])
        results = []
        bppbase = 0
        for future, rdo_l in futures:
            res = future.result()
            if rdo_l == 0:
                bppbase = res['bpp']
            res['rdo_l'] = rdo_l
            res['ratio'] = res['bpp'] / bppbase
            results.append(res)
        return results

fields = ['bpp', 'rms', 'psnr', 'ratio']
fig, axs = plt.subplots(1, len(fields) + 1, layout='constrained')
fig.set_figwidth(5 * (len(fields) + 1))
lines = []

for path in args.files:
    print('Processing', path)
    results = stats(path)
    etcbase = compress(path, ["-q", "192"]) if args.etcbase else None

    for idx, field in enumerate(fields):
        line, = axs[idx].plot([r['rdo_l'] for r in results], [r[field] for r in results])
        if etcbase and field in etcbase:
            axs[idx].axhline(etcbase[field], color=line.get_color(), linestyle='dotted')
        if idx == 0:
            lines.append(line)

    axs[len(fields)].scatter([r['ratio'] for r in results], [r['psnr'] for r in results], color=line.get_color())

for idx, field in enumerate(fields):
    axs[idx].set_title(field)

axs[len(fields)].set_title('psnr vs ratio')

fig.legend(lines, [os.path.basename(path) for path in args.files], loc='outside right upper')

plt.savefig(args.graph)
