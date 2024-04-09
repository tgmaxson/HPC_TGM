from enum import unique
import matplotlib.pyplot as plt
import numpy as np


lines = open("performance_O3.txt").readlines()
data = np.zeros((len(lines), 5))

for index, line in enumerate(lines):
    line = line.replace(",", " ")
    parts = line.split()
    data[index] = (int(parts[3]), int(parts[5]), int(parts[7]), int(parts[11]), float(parts[9]))

# Function to calculate the average for duplicated entries
def average_duplicates(data):
    unique_keys, indices, counts = np.unique(data[:, :4], axis=0, return_inverse=True, return_counts=True)
    avg_data = np.zeros((unique_keys.shape[0], data.shape[1]))
    avg_data[:, :4] = unique_keys
    for i in range(len(unique_keys)):
        avg_data[i, 4] = np.mean(data[indices == i, 4])
    return avg_data

data = average_duplicates(data)

unique_sizes = np.unique(data[:, 1]) 

unique_sizes = [x for x in unique_sizes if x > 1000]

max_threads = int(max(data[:, 0]))  # Maximum number of threads

for size in unique_sizes:
    subset = data[data[:, 1] == size]

    sorted_subset = subset[np.argsort(subset[:, 0])]
    plt.plot(sorted_subset[:, 0], sorted_subset[:, 4], label=f'Size {int(size)}x{int(size)}')

plt.xlabel('Ranks')
plt.ylabel('Performance (Time)')
plt.title('Performance vs. Ranks for Different Sizes')
plt.legend()
plt.tight_layout()
plt.savefig("performance_time_O3.png")
plt.show()

for size in unique_sizes:
    subset = data[data[:, 1] == size]
    sorted_subset = subset[np.argsort(subset[:, 0])]

    one_thread_performance = sorted_subset[sorted_subset[:, 0] == 1][:, 4]
    if one_thread_performance.size > 0:
        one_thread_performance = one_thread_performance[0]  # Take the first element if multiple entries
        speedup = one_thread_performance / sorted_subset[:, 4]  # Calculate speedup
        plt.plot(sorted_subset[:, 0], speedup, label=f'Size {int(size)}x{int(size)}')
        if size == 4000:
            print(sorted_subset[:, 0])
            print(speedup)

threads = np.arange(1, max_threads + 1)
plt.plot(threads, threads, '--', label='Linear Scaling', color='gray')

plt.xlabel('Ranks')
plt.ylabel('Speedup')
plt.ylim(0, None)
plt.title('Speedup vs. Ranks for Different Sizes')
plt.legend()
plt.tight_layout()
plt.savefig("performance_speedup_O3.png")
plt.show()

for size in unique_sizes:
    subset = data[data[:, 1] == size]
    sorted_subset = subset[np.argsort(subset[:, 0])]

    one_thread_performance = sorted_subset[sorted_subset[:, 0] == 1][:, 4]
    if one_thread_performance.size > 0:
        one_thread_performance = one_thread_performance[0]  # Take the first element if multiple entries
        speedup = one_thread_performance / sorted_subset[:, 4]  # Calculate speedup
        plt.plot(sorted_subset[:, 0], speedup / sorted_subset[:, 0], label=f'Size {int(size)}x{int(size)}')

plt.xlabel('Ranks')
plt.ylabel('Efficiency (Speedup / Ranks)')
plt.ylim(0, None)
plt.title('Efficiency vs. Ranks for Different Sizes')
plt.legend()
plt.tight_layout()
plt.savefig("performance_efficiency_O3.png")
plt.show()

for size in unique_sizes:
    subset = data[data[:, 1] == size]
    sorted_subset = subset[np.argsort(subset[:, 3])]

    one_thread_performance = sorted_subset[sorted_subset[:, 0] == 1][:, 4]
    if one_thread_performance.size > 0:
        one_thread_performance = one_thread_performance[0]  # Take the first element if multiple entries
        speedup = one_thread_performance / sorted_subset[:, 4]  # Calculate speedup
        plt.plot(sorted_subset[:, 3], speedup / sorted_subset[:, 0], label=f'Size {int(size)}x{int(size)}')

plt.xlabel('Block Size')
plt.ylabel('Efficiency (Speedup / Threads)')
plt.ylim(0, None)
plt.title('Efficiency vs. Block Size for Different Sizes')
plt.legend()
plt.tight_layout()
plt.savefig("performance_block_O3.png")
plt.show()
