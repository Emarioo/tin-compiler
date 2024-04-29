A compiler with a simple language called **Tin**.

# Performance
During all tests, as few programs are running on the computer as possible. No browsers, only Tracy Profiler and a text editor.

Tested with `AMD Ryzen 7 4800H i7-4790K CPU, 2.9 GHz (base), 8/16 cores` (laptop from 2021)

(the laptop runs faster when power is connected, all tests using this laptop was run when the laptop was connected for consistency)

![](docs/perf_cpu1_multiple_threads.png)

![](docs/perf_cpu1_single_thread.png)

Tested with `i7-4790K CPU, 4.00 GHz, 4/8 cores` (old computer from 2013-2014)

![](docs/performance_mutex_overhead.png)

![](docs/performance_single_thread.png)