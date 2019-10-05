# NS-SHAFT-Robot
A robot plays NS-SHAFT automatically.


### Environment setup
This code is implementation on Windows 10 and the programs are written in C++ using GCC version 8.1.0 and OpenCV version 4.1.1. 
First, download and install MinGW-W64 GCC-8.1.0 using x86_64-posix-seh instead of x86_64-win32-seh, because the win32 thread model doesn't support C++11 threading features and OpenCV-4.1.1. [Download](https://sourceforge.net/projects/mingw-w64/files/)

Second, download and install opencv OpenCV-MinGW-Build. [Download](https://github.com/huihut/OpenCV-MinGW-Build)

Third, download this project and modify OPENCVPATH in the Makefile. 

Finally, execute the command ```make``` in the path of the Makefile to compile the code.