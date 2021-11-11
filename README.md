# Introduction #
C++ for scripts (cpp4scripts or c4s for short), is a library of classes and functions that
make it easy to write medium and large size "scripts" with C++. Small add-hock scripts are more feasible to
write with Bash (or possibly Perl, Python, etc) but as the complexity of the script increases it helps to
have things like classes, exceptions, debuggers and logging to name a few. These scripts still need to be
compiled but with current compilers this takes only second or two. Trivial price to pay for extra speed
and power.

Using C++ does create more source code than your average script language as far as the line or character count is
concerned. This library tries to mitigate the gap and does pretty good job at it. And if you work mostly with C++
then those few extra lines that are needed are faster to write than trying to learn intricasies of Bash for example.

# Building the library
C4S library includes classes to create custom make files or build scripts. Instead of providing scripts or make files 
for this library we make use of the library's functionality to create build executable that in turn can be used to 
create and install the library itself. 

To create build-binary in Lihux / Mac:
> g++ -o build build.cxx -Wall -Wno-unused-result -pthread -fexceptions -fno-rtti -fuse-cxa-atexit -O2 -std=c++17 -lstdc++

To create build-exe in Windows:
> cl /Febuild.exe /O2 /MD /EHsc /W2 build.cxx Advapi32.lib

Once build-binary is created we can use it for the library:
> ./build -deb
> ./build -rel
> ./build -install [target]

# Releases #
## Version 0.21 ##
- Remove obsolete dynamic build directories from build.cpp


