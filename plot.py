from serial import Serial

import matplotlib.pyplot as plt
import matplotlib.animation as animation

import time

fig = plt.figure()

# sensors = ["throttle", "voltage high", "voltage low"]
# maxVals = [5,          28,             14]
sensors = ["current"]
maxVals = [2000]

def sensorIndex(sensor):
    i = 0
    while sensors[i] != sensor:
        i += 1
    return i

def resistance():
    return sensors[sensorIndex("voltage")] / sensors[sensorIndex("current")]

calcVals = ["resistance"]
calcFuncs = []
maxCalcVals = [8]
yss = []
xs = []
axs = []
txtAx = None

def colours(i):
    colourOptions = ["red", "green", "blue"]
    return colourOptions[i % len(colourOptions)]

logFile = "logs/log.csv"

window = 30
startTime = time.time()

logLines = []
maxLogLines = 50
txtAx = fig.add_subplot(1, len(sensors) + 1, len(sensors) + 1)
txtAx.set_title("log")

with open(logFile, "w") as log:
    log.write("time")

    for i, sensor in enumerate(sensors):
        log.write(f",{sensor}")
        yss.append([])
        ax = fig.add_subplot(1, len(sensors) + 1, i + 1)
        ax.set_title(sensor)
        axs.append(ax)


    log.write("\n")
    log.flush()

    def animate(i):

        dataType = None
        data = None
        dataLine = None
        try:
            if serial.in_waiting > 0:
                dataLine = serial.readline().decode().strip()

                logLines.append(dataLine)
                if len(logLines) > maxLogLines:
                    newLogLines = logLines[-maxLogLines:]
                    logLines.clear()
                    logLines.extend(newLogLines)

                logText = logLines[0]
                for line in logLines[1:]:
                    logText = f"{logText}\n{line}"

                txtAx.clear()
                txtAx.text(0, 0, logText, transform = txtAx.transAxes, verticalalignment = "bottom", wrap = True)

                data = [float(i) for i in dataLine.split(",")]
        except:
            pass

        if data is not None:
            if len(data) != len(sensors):
                raise ValueError("Data length does not match number of listed sensors")

            newTime = time.time() - startTime                   
            xs.append(newTime)

            log.write(f"{newTime},{dataLine}\n")
            log.flush()

            windowStart = 0
            while xs[windowStart] < newTime - window:
                windowStart += 1

            # print(windowStart)

            newXs = xs[windowStart:]

            xs.clear()
            xs.extend(newXs)

            for i, val in enumerate(data):
                yss[i].append(val)
                newYs = yss[i][windowStart:]
                yss[i].clear()
                yss[i].extend(newYs)

            for i, ys in enumerate(yss):
                axs[i].clear()
                axs[i].set_title(sensors[i])
                axs[i].plot(xs, ys, color=colours(i))
                axs[i].set_xlim(newTime - window, newTime)
                axs[i].set_ylim(0, maxVals[i])

    serial = Serial("/dev/ttyACM0", 115200)

    ani = animation.FuncAnimation(fig, animate, interval = 100)
    plt.show()