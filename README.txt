SYSC4001 Assignment 3
Author: Zakaria Ismail
S#: 101143497

README

Overview

This is a calculator program using the client-server paradigm.
Multiple clients can use the server at a time; up to 100. Up to
98 values can be stored for each client.

Compile:

To compile the program, type 'make clean' and then 'make in the terminal
window.

Run:

To run the program, open two terminal windows and run
the server on one and the client in the other.

Run server:

To run the server, type './calculator' on the 1st terminal.
There will be a 'Waiting for message...' displayed.

Run client:

To run the client, type './user' on the 2nd terminal. 
There will be an input prompt.

Debug Mode:

To run the program in debug mode, where debug prints are
displayed, go to common.h,set the 'DEBUG' flag to 1, and
compile the program using 'make clean' and 'make', then run client and server
again.

The available commands to run are:
    - insert <arg>
        Inserts arg into server data
    - delete <arg>
        Deletes all instances of arg in server data
    - sum
        Gets the sum of all values in server data
    - average
        Gets the average of all values in server data
    - min
        Gets the minimum of all values in server data
    - median
        Gets the median of all values in server data
    - end

The commands are non-case sensitive. 'arg' is an integer
argument. 

The program works with multiple clients and each client
has their own server data, which gets cleared when they
send the 'end' command.
When the last client sends an 'end' command, the server
stops running. Note that if the first client's first command
is 'end' the server will not end because they do not have a
session w/ the server, so the server will ignore the command.

