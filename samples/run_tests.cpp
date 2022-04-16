#include <fstream>

typedef bool (*tfptr)();

struct TestItem {
	TestItem( tfptr fn, const char* s) {
		test = fn;
		subject = s;
	}
	bool OK() {
		return test ? true : false;
	}

  	tfptr test;
  	const char* subject;
};

std::ofstream c4slog;

bool is_set(int argc, char** argv, const char* name)
{
	for (int n=1; n<argc; n++) {
		if (!strncmp(argv[n], name, strlen(name)))
			return true;
	}
	return false;
}
const char* get_value(int argc, char** argv, const char* name)
{
	for (int n=1; n<argc; n++) {
		if (!strncmp(argv[n], name, strlen(name))) {
			if (n+1 < argc) return argv[n+1];
		}
	}
	return "";
}
int run_tests(int argc, char **argv, TestItem *tests)
{
	int ndx;

	c4slog.open("unit_test.log");

    try {
	    if (is_set(argc,argv,"-a")) {
	    	int count_OK = 0;
	    	int count_FAIL = 0;
	    	for (ndx=0; tests[ndx].OK(); ndx++) {
	    		cout << ndx+1 << " " << tests[ndx].subject;
	    		if (tests[ndx].test()) {
	    		  	cout << "\tOK\n";
	    		  	count_OK++;
	    		}
	    		else {
	    			cout << "\tFail\n";
	    			count_FAIL++;
	    		}
	    	}
	    	cout << "------------\n";
	    	cout << "Pass/fail = "<<count_OK<<'/'<<count_FAIL<<" = "<< 100*count_OK/(count_OK+count_FAIL) << "%%\n";
	    }
	    if (is_set(argc,argv,"-l")) {
	    	for (ndx=0; tests[ndx].OK(); ndx++)
	    		cout << '(' << ndx+1 << ") " << tests[ndx].subject << '\n';;
	    	cout << endl;
	    	return 0;
	    }
	    if (is_set(argc,argv,"-t")) {
	    	int t_index = stol(get_value(argc,argv,"-t"));
	    	for (ndx=0; tests[ndx].OK(); ndx++) {
	    		if (ndx+1 == t_index) {
	    			if (tests[ndx].test()) cout << "Test OK\n";
	    			else cout << "Test Failed\n";
	    			break;
	    		}
	    	}
	    	if (!tests[ndx].OK()) {
	    		cerr << "Unknown test index\n";
	    		return -1;
	    	}
	    }
    } catch(const runtime_error& re) {
        c4slog << "Unit test failure: " << re.what() << endl;
        return 1;
    }
    return 0;
}