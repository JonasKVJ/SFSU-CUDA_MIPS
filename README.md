# SFSU-MIPS
Several courses on the MIPS ISA taken at San Francisco State University.
The first course was Machine Structures taken Fall 2015 and the second was an advanced elective class called Computer Organization, taken during the Spring-18 semester. 
Overview of assignments: 
- Project 1: Optimization of pipelined instructions through reorganization, both in case of a single-issue and a multi-issue processor. One part of the project also involves unrolling a loop, such that one new iteration consists of four old ones. 
- Project 2: Branch prediction simulator, which scans a trace file of any size and determines its branch statistics. Project2_system1.c is a simplified simulator, which always predicts forward branches as taken and backward branches as not taken. Project2_system2.c is a much more advanced and realistic simulator, which directly maps/hashes a 2-bit Prediction Buffer to the PC, where PC is read from the trace file. It also maps a Branch Target Buffer to the PC with direct mapping and as part of the mapping process, a bit mask was used to cut off relevant bits of the PC to look up entries in the Prediction Buffer and the BTB. Bits fields were used to limit the values of the 2-bit predictors and a verbose mode was implemented, which allowed for extra information to be printed by submitting an optional -v argument.  

General topics in the two courses: Binary and Hexadecimal, Translating C++ to MIPS Assembly, Logic Circuits such as D Flip-flops, Multiplexors, Registers, Clocks; Pipelining, Caches (Direct Mapping), Advanced Pipelining (8 stage pipelines vs. 5 stage ones), Loop Unrolling, Exceptions & Interrupts, Stalls, Speculation, Datapaths & Control Signals. 

Note: This repository will be updated progressively, as more projects are completed during the semester 
