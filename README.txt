# Memory-simulator

Memory simulator
Authored by Simcha Corcos
212328199

==Description==

The program is a simulation of processor approaches to memory (RAM), We use the mechanism "paging" that allow running a program when only a portion of it is in the memory.
The virtual memory is divided into pages ,this pages are brought to the main memory (RAM) by need.
In this program we use one proccess as the virtual memory, the simulation will be implemented by  two main approaches,load an address to the main memory and store address in the main memory by using the Hard disk.

Program DATABASE:
struct database that contain sub databases, the sub databases are:
1.page_table=array of structs, the page table serves as a table of contents ,we can get information about the RAM,address,and swap file.
2.swap_fd= file descriptor,hold the access to the swap file , the swap file simulate the Hard disk .
3.program_fd= file descriptor,hold the access to the executable file, this file simulate a process! 

functions:
two main functions:
1.load- this method receive an address, and loads the value in this address
2.store- this method receive an address, and value and put the value in this address

Program Files:
mem_sim.cpp 
mem_sim.h
main.cpp

How to compile?
compile:  gcc -Wall main.cpp sim_mem.cpp -o main
run: ./main

Input:
no input

Output:
main memory (RAM)
swap file
value - that the load function return


