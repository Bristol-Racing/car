import argparse
import os
import math

import matplotlib.pyplot as plt

from scipy.linalg import lstsq

fig = plt.figure()
ax = fig.add_subplot()

argParser = argparse.ArgumentParser()
argParser.add_argument("-i", "--input", type=str, help="input folder containing voltage logs", required=True)
argParser.add_argument("-m", "--min", type=int, help="minimum voltage to calculate from", required=True)

args = argParser.parse_args()

inputDir = os.path.abspath(args.input)

files : list[str] = os.listdir(inputDir)

xs = []
ys = []

# y = a + b * x + c * (x ** 2)

A = []
B = []

maxVal = 0

for file in files:
    valid = True
    parts = file.split(".")
    ext = parts[1]
    parts = parts[0].split("_")

    if ext != "csv":
        valid = False

    if len(parts) != 2:
        valid = False

    if parts[0] != "log":
        valid = False

    unit = parts[1][-1]
    val = parts[1][:-1]

    if unit != "V":
        valid = False

    try:
        val = int(val)
    except:
        valid = False

    if not valid:
        print(f"skipping invalid file: {file}")
    elif val < args.min:
        pass
    else:
        if val > maxVal:
            maxVal = val

        with open(os.path.join(inputDir, file)) as csv:
            first = True
            lines = csv.readlines()
            for line in lines:
                if first:
                    first = False
                    continue

                reading = float(line.split(",")[1])

                xs.append(reading)
                ys.append(val)

                A.append([reading])
                # A.append([1, val, val ** 2, val ** 3])
                # A.append([1, math.exp(val)])

    # print(f"file found with voltage: {val}")

p, res, rank, s = lstsq(A, ys)
print(p)

x = 0
fitXs = []

while x < 1024:
    fitXs.append(x)
    x += 1

fitYs = [p[0] * x for x in fitXs]
# fitYs = [p[0] + p[1] * x + p[2] * x ** 2 + p[3] * x ** 3 for x in fitXs]
# fitYs = [p[0] + p[1] * math.exp(val) for x in fitXs]

ax.scatter(xs, ys)
ax.plot(fitXs, fitYs)
plt.show()