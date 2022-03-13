/* Simply echo arguments 
To build macos: clang -O0 -o echo -lc++ echo.cpp
To biuld linux: clang -O0 -o echo -lstdc++ echo.cpp
*/
#include <iostream>
#include <string>
using namespace std;

int main(int argc, char **argv)
{
    cout<<"Echoing arguments:\n";
    for(int ndx=0; ndx<argc; ndx++) {
        cout<<"  ["<<ndx<<"] = "<<argv[ndx]<<'\n';
    }
    cout << "Echo stdin:\n";
    string line;
    while(getline(cin, line)) {
        cout << line  << '\n';
    }
    return 0;
}
