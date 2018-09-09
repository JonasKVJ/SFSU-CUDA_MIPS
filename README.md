# SFSU-CUDA-MIPS
Several courses on the MIPS ISA taken at San Francisco State University.
The first course was Machine Structures taken Fall 2015 and the second was an elective class called Computer Organization, taken during the Spring-18 semester. 

Overview of assignments: 
- Project 1: Optimization of pipelined instructions through reorganization, both in case of a single-issue and a multi-issue processor. One part of the project also involves unrolling a loop, such that one new iteration consists of four old ones. 
- Project 2: Rewrote small and simple CPU programs into GPU versions, which were split into a part A and B. The part A involved rewriting a program which found the max value of the columns of a n by n matrix into a coalesced GPU version. Part B included initializing a bunch of 3D particle coordinates with RGB colors and then counting how many red particles were within a distance of 0.05 of each individual particle. The majority of each part were written by Dr. William Hsu, and each student was expected to fill out and submit the remaining parts (as indicated in the code). 

General topics in the two courses: Binary and Hexadecimal, Translating C/C++ to MIPS Assembly, Logic Circuits such as D Flip-flops, Multiplexors, Registers, Clocks; Pipelining, Caches (Direct Mapping), Advanced Pipelining (8 stage pipelines vs. 5 stage ones), Loop Unrolling, Exceptions & Interrupts, Stalls, Speculation, Datapaths & Control Signals, CUDA GPU programming. 
