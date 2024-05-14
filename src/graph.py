
import matplotlib.pyplot as plt

# Each execution was measured by starting a compiler process three times and taking the median execution time.

# 979630 lines, 88 files, 49.70 MB

# Times for slow version
#   1 threads 5.644 sec (173558 lines/s) 8.80 MB/s
#  16 threads 64.878 sec (15099 lines/s) 784.37 KB/s

# Times for optimized version
#  1 threads 5.459 sec (179451 lines/s) 9.10 MB/s
#  2 threads 3.057 sec (320484 lines/s) 16.26 MB/s
#  4 threads 1.802 sec (543490 lines/s) 27.57 MB/s
#  6 threads 1.442 sec (679127 lines/s) 34.45 MB/s
#  8 threads 1.260 sec (777508 lines/s) 39.44 MB/s
# 10 threads 1.152 sec (850250 lines/s) 43.13 MB/s
# 12 threads 1.146 sec (855116 lines/s) 43.38 MB/s 
# 14 threads 1.101 sec (890124 lines/s) 45.16 MB/s
# 16 threads 1.140 sec (859669 lines/s) 43.61 MB/s

arr = [
     1, 5459,
     2, 3057,
     4, 1802,
     6, 1442,
     8, 1260,
    10, 1152,
    12, 1146,
    14, 1101,
    16, 1140,
]

# Data
N = []
T = []
for i in range(0,int(len(arr)/2)):
    N.append(arr[2*i])
    T.append(arr[2*i + 1])

# Calculate speedup (S)
T1 = T[0]  # Time taken for 1 CPU
S = [T1 / tn for tn in T]  # Speedup = T1 / Tn

# Calculate efficiency (E)
E = [s / n for s, n in zip(S, N)]  # Efficiency = Speedup / No. Of CPUs


# Plotting
plt.figure(figsize=(10, 6))

# Plotting Compile time (ms)
plt.subplot(3, 1, 1)
plt.plot(N, T, marker='o', color='b', label='Compile time (ms)')
plt.title('Compile time vs Threads')
plt.ylim(0,6000)
plt.xlabel('Threads (N)')
plt.ylabel('Compile time (ms)')
plt.grid(True)
plt.legend()

# Plotting Speedup
plt.subplot(3, 1, 2)
plt.ylim(0,6)
plt.plot(N, S, marker='o', color='r', label='Speedup (T1/Ti)')
plt.title('Speedup vs Threads')
plt.xlabel('Threads (N)')
plt.ylabel('Speedup (T1/Ti)')
plt.grid(True)
plt.legend()

# Plotting Efficiency
plt.subplot(3, 1, 3)
plt.plot(N, E, marker='o', color='g', label='Efficiency (S/N)')
plt.title('Efficiency vs Threads')
plt.ylim(0,1.4)
plt.xlabel('Threads (N)')
plt.ylabel('Efficiency (S/N)')
plt.grid(True)
plt.legend()



plt.tight_layout()
plt.show()
