CONTRIBUTIONS:

For this project, we worked together for all works.
Yoohyuk Chang (ychang82): Worked together on Fork/join computation, Memory-mapped file I/O, and experiments/analysis.
Yongjae Lee (ylee207): Worked together on Fork/join computation, Memory-mapped file I/O, and experiments/analysis.





REPORT: EXPERIMENTS AND ANALYSIS:

Below is the raw result file (excluding the 'user' and 'sys' time) by running './run_experiments.sh'

1. First Trial of running ./run_experiments.sh

    Test run with threshold 2097152
    real    0m0.395s

    Test run with threshold 1048576
    real    0m0.234s

    Test run with threshold 524288
    real    0m0.170s

    Test run with threshold 262144
    real    0m0.136s

    Test run with threshold 131072
    real    0m0.139s

    Test run with threshold 65536
    real    0m0.139s

    Test run with threshold 32768
    real    0m0.155s
    
    Test run with threshold 16384
    real    0m0.163s


2. Second Trial of running ./run_experiments.sh

    Test run with threshold 2097152
    real    0m0.400s

    Test run with threshold 1048576
    real    0m0.235s

    Test run with threshold 524288
    real    0m0.157s

    Test run with threshold 262144
    real    0m0.136s

    Test run with threshold 131072
    real    0m0.141s

    Test run with threshold 65536
    real    0m0.150s

    Test run with threshold 32768
    real    0m0.152s

    Test run with threshold 16384
    real    0m0.160s


3. Third Trial of running ./run_experiments.sh

    Test run with threshold 2097152
    real    0m0.426s

    Test run with threshold 1048576
    real    0m0.279s

    Test run with threshold 524288
    real    0m0.165s

    Test run with threshold 262144
    real    0m0.138s

    Test run with threshold 131072
    real    0m0.137s

    Test run with threshold 65536
    real    0m0.142s

    Test run with threshold 32768
    real    0m0.202s

    Test run with threshold 16384
    real    0m0.161s


For better readability, We will organize the three result files as shown below.

<Formatted First Result>
    Threshold (Bytes)	Time Taken (seconds)
    2,097,152	        0.395
    1,048,576	        0.234
    524,288	            0.170
    262,144	            0.136
    131,072	            0.139
    65,536	            0.139
    32,768	            0.155
    16,384	            0.163


    Threshold (Bytes)	Time Taken (seconds)
    2,097,152	        0.400
    1,048,576	        0.235
    524,288	            0.157
    262,144	            0.136
    131,072	            0.141
    65,536	            0.150
    32,768	            0.152
    16,384	            0.160


    Threshold (Bytes)	Time Taken (seconds)
    2,097,152	        0.426
    1,048,576	        0.279
    524,288	            0.165
    262,144	            0.138
    131,072	            0.137
    65,536	            0.142
    32,768	            0.202
    16,384	            0.161


The parsort program utilizes a fork/join parallel computation model. 
As the threshold lowers, the program increases the degree of parallelism 
by creating more processes to sort smaller segments of the data file. 
Each process performs its sorting task independently, which can be 
scheduled by the OS kernel on different CPU cores. This parallel execution 
significantly speeds up the sorting process, especially when the number of 
processes aligns well with the number of available CPU cores.

The best performance was observed at a threshold of 262,144, beyond which 
the sorting time began to increase slightly with more parallelism. This can 
be attributed to the fact that the optimal level of parallelism for the given 
hardware was reached at this point. Beyond this threshold, the number of processes 
exceeded the number of CPU cores, leading to diminishing returns due to overheads 
such as context switching and process management.

When the threshold was set too low, resulting in a very high degree of parallelism, 
the sorting time slightly increased. This is because, beyond the optimal parallelism 
level, the overheads associated with process creation, inter-process communication, 
and the management of a larger number of processes outweighed the benefits of 
parallel execution. Furthermore, the merge phase of the sort, being inherently 
sequential, became a bottleneck as it had to deal with a higher number of smaller 
sorted arrays.

The observed pattern in sorting times reflects a common characteristic of parallel 
computing: while parallelization can significantly improve performance, there is 
an optimal point beyond which further parallelization leads to inefficiencies. 
This is due to the additional overheads introduced by managing more processes and 
the limitations of hardware resources, such as CPU cores and memory bandwidth.