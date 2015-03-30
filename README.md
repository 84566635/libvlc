# libvlc
Library functions for the visible light communication (VLC) implementation built as a bachelor's thesis during the spring 2015.

Directories:
--------------
The code is structured in the following directories:
- bin: Contains the finished, compiled program.
- bin/src: Contains the main file for use in the finished programme. This file is only used when "make install" is used.
- build: Contains build files, such as the compiled library containing all functions written in the src directory.
- src: Contains all source files for library functions. The source files are sorted in directories.
- tests: Contains code for the unit tests and the test log.

Source code:
--------------
Source code for all functions is kept in the src directory. The main function for the finished exectuable is kept in bin/src. All source files are kept in directories which should be self-explanatory.

The source code is sorted into files based on which layer the code operates on. The byte layer functions has one file for example. The source files themselves contain more detailed documentation.

Compiling:
--------------
- Compile the library functions by typing "make" in the root directory.
- Run the unit tests by typing "make tests" in the root directory.
- Compile the finished program by typing "make install" in the root directory.

The typical use case is to write:
```
make            #Compile library functions
make tests      #Run unit tests to make sure everything is working
make install    #Create the executable
./bin/cvlc      #Run the executable
```
TODO:
--------------
- Improve the ACK frame implementation. ACKs should be tied to individual packets rather than be a general signal. This will make full duplex communication easier.
- Add CRC checks to packets.
- Add unique sequence numbers to every packet. This way packets can be sent out of order and ACKs can be tied to individual packets.
- Add code for retransmitting the packet if CRC check fails.
- Implement the LINK layer in some environment that can handle hard real-time constraints.
- Examine Xenomai project for performing real-time tasks.