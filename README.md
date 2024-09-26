Possible output (on my machine; fairly consistently, but sometimes it deviates from the actual output).
```
assessed cache line size: 64
assessed first tag index: 21
assessed associativity: 12
```

The code must be run in the debug configuration of cmake (compilation flags `-g -std=gnu++20`).

To print more specific statistics, uncomment lines printing per-step metrics.

Thus, the assessed cache size is $2^{21-6} \times 2^6 \times 12 \approx 25$ Mb, which is approximately twice as large as my actual L3 cache.