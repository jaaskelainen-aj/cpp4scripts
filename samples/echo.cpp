/* Simply echo arguments 
To build: clang -O0 -o echo -lc++ echo.cpp
*/
#include <iostream>
using namespace std;

int main(int argc, char **argv)
{
    cout<<"Echoing arguments:\n";
    for(int ndx=0; ndx<argc; ndx++) {
        cout<<"  ["<<ndx<<"] = "<<argv[ndx]<<'\n';
    }
    return 0;
}
