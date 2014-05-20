Markovian System Simulation
===========================

Compiling
---------
All you need is `main.cpp`, make and a recent C++11 compiler and Standard Library. Just run `$ make`.

Usage
-----
After compiling, run `$ ./main` to get a useful usage message.

To simulate a system with (N, \mu, \lambda, K) = (10, 8, 4, 6) where the simulation would check for convergence every 1000000 events and halt at 100 cycles unless the tolerance has already dropped below 0.5%, all while giving a verbose (1) output, give `$ ./main 10 8 4 6 1000000 100 0.5 1`

Script
------
To simulate a system under configurations of varying K and \lambda, use the `run.sh` script. It automates a lot of work and produces some neat (if preliminary) statistics.
