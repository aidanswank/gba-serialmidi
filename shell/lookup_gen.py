import math

def truncate(number, digits) -> float:
    stepper = 10.0 ** digits
    return math.trunc(stepper * number) / stepper

def freq2rate(freq):
    rate = ((2048*freq)-131072)/freq
    return rate

def midi2freq(note):
    freq = 440.0 * pow(2, (note - 69) / 12.0)
    return freq

f = open("lookup.txt", "w+")

for note in range(128):
    freq = truncate(midi2freq(note), 4)
    rate = int(freq2rate(freq))
    f.write(str(rate)+',\n')
