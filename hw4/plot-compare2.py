import matplotlib.pyplot as plt
import numpy as np

thread_vals = [ 1. ,  2. ,  4. ,  8. , 12. , 16. , 20. , 24. , 28.]
openmp = [1.    ,     1.94814376, 3.51000542, 5.62902385, 6.2670701,  6.55621079, 7.19517147, 6.9463706,  7.10011583]
blocking = [1. , 1.70658713, 3.65920789, 6.03175329, 7.11533967, 6.63646273, 7.6627618,  6.16967209, 7.79815429]
nonblocking = [1.         , 1.672007,   3.64241605, 5.09933547, 7.23129003, 7.70275802, 7.65674244, 7.70901879, 7.78648365]

plt.plot(thread_vals, blocking, label=f'Size 4000x4000 (MPI Blocking)')
plt.plot(thread_vals, nonblocking, label=f'Size 4000x4000 (MPI Non-blocking)')
plt.plot(thread_vals, openmp, label=f'Size 4000x4000 (OpenMP)')

threads = np.arange(1, 28)
plt.plot(threads, threads, '--', label='Linear Scaling', color='gray')

plt.xlabel('Ranks')
plt.ylabel('Speedup')
plt.ylim(0, None)
plt.title('Speedup vs. Ranks wrt. Parallelization Method')
plt.legend()
plt.tight_layout()
plt.savefig("compare2_speedup.png")
plt.show()
