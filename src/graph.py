
import matplotlib.pyplot as plt

# These measures were taken by running each thead test
# 5 times in a new process. I/We did not run all tests
# in one process because we seem to get worse results the longer
# the tests run. Probably due to the operating system decreasing the priority on our process, letting it execute less often -> worse times since we're doing wall clock time.

# Times for optimized version (all see Config.h for optimized options)
#   1, 4608.091 ms, 11.308 MB/s, (52.109550 MB)
#   2, 2351.428 ms, 22.161 MB/s, (52.109550 MB)
#   4, 1595.350 ms, 32.663 MB/s, (52.109550 MB)
#   6, 1268.404 ms, 41.083 MB/s, (52.109550 MB)
#   8, 1201.181 ms, 43.382 MB/s, (52.109550 MB)
#  10, 1203.225 ms, 43.308 MB/s, (52.109550 MB)
#  12, 1267.452 ms, 41.114 MB/s, (52.109550 MB)
#  14, 1228.820 ms, 42.406 MB/s, (52.109550 MB)
#  16, 1288.915 ms, 40.429 MB/s, (52.109550 MB)

# Times for slow version
#   1, 4632.633 ms, 11.248 MB/s, (52.109550 MB)
#   takes to long
#  16, 89.311 sec (10968 lines/s) 569.79 KB/s

arr = [
     1, 4608.091,
     2, 2351.428,
     4, 1595.350,
     6, 1268.404,
     8, 1201.181,
    10, 1203.225,
    12, 1267.452,
    14, 1228.820,
    16, 1288.915,
]

for i in range(0,int(len(arr)/2)):
    print(979630 / (arr[2*i+1]/1000))

# Data
N = []
T = []
for i in range(0,int(len(arr)/2)):
    N.append(arr[2*i])
    T.append(arr[2*i + 1])

# Calculate speedup (S)
T1 = T[0]  # Time taken for 1 CPU
S = [T1 / ti for ti in T]  # Speedup = T1 / Ti

# Calculate efficiency (E)
E = [s / n for s, n in zip(S, N)]  # Efficiency = Speedup / No. Of CPUs


# Plotting
plt.figure(figsize=(10, 6))

# Plotting Compile time (ms)
plt.subplot(3, 1, 1)
plt.plot(N, T, marker='o', color='b', label='Compile time (ms)')
plt.title('Compile time vs Threads')
plt.ylim(0,5500)
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
