#define C4S_DEBUGTRACE
#include "../c4s_all.hpp"
using namespace c4s;

int main(int argc, char **argv)
{
    path_iterator pi;
    path_list all(path("./"), "*");

    /*
    string s1("foo"), s2("bar");
    cout << "cmpare foo-bar:"<<s1.compare(s2)<<'\n';
    cout << "cmpare bar-foo:"<<s2.compare(s1)<<'\n';
    path p1("foo"), p2("bar");
    cout << "compare p-foo to p-bar:"<<p1.compare(p2,CMP_BASE)<<'\n';
    cout << "compare p-bar to p-foo:"<<p2.compare(p1,CMP_BASE)<<'\n';
    */

    cout << "-- Unsorted --\n";
    for(pi=all.begin(); pi!=all.end(); pi++) {
        cout << pi->get_base()<<'\n';
    }
    cout << "\n-- Sorted --\n";
    all.sort(path_list::ST_PARTIAL);
    for(pi=all.begin(); pi!=all.end(); pi++) {
        cout << pi->get_base()<<'\n';
    }
    cout << "--Done\n";
    return 0;
}
