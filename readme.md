# HW3 - Building an HTTP Client

## Description

This is my third assignment from my Computer Networks class at Drexel University.

The assignment gives as us a lot of starter code helpful to creating TCP connections, and making HTTP requests for closed and keep-alive connections.

We finished the implementation of client-cc.c, where we create a closed TCP connection that ends after the initial first request is made.

We also finished the implementation of client-ka.c, where we implement a keep-alive TCP connection, which attempts to stay alive and retries connecting to the target host five times before closing its socket and connection.

The connections that client-cc.c and client-ka.c are for arbitary html pages. If you run the files you will see the contents of these html pages.

The purpose of the assignment was to gain familiarity in implementing these two approaches, and also to compare the speeds of the two approaches. You can find the results of running my code in timing.txt.

### If you wish to run the code yourself and get the runtime, you can run the following commands in the terminal at the root of this repository:

make all (compiles both client-cc.c and client-ka.c)

then run:

make run-cc (for running client-cc.c)

make run-ka (for running client-ka.c)

