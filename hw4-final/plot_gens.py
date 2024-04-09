import matplotlib.pyplot as plt
import numpy as np

# Data: number of rows for each rank and total columns
gens = [1000, 2000, 3000, 4000]
time = [0.996, 1.994, 2.988, 3.994]

# Create a figure and axis for the plot
fig, ax = plt.subplots(figsize=(6, 6))

# Set the labels and title of the plot
ax.set_xlabel('Generations')
ax.set_ylabel('Time (seconds)')
ax.plot(gens, time)

plt.savefig("gen_scale.png")
