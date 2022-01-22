
typedef void (*tfptr)();

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

program_arguments args;

int run_tests(int argc, char **argv, TestItem *tests)
{
	int ndx;

    args += argument("-t", true,  "Sets VALUE as the test to run.");
    args += argument("-a", false, "Runs all tests");
    args += argument("-l", false, "List all tests in this unit test binary.");
    args += argument("--help", false, "Show this help");

    try {
	    args.initialize(argc,argv,1);
	    if (args.is_set("--help")) {
	    	args.usage();
	    	return 0;
	    }
	    if (args.is_set("-a")) {
	    	for (ndx=0; tests[ndx].OK(); ndx++)
	    		tests[ndx].test();
	    }
	    if (args.is_set("-l")) {
	    	for (ndx=0; tests[ndx].OK(); ndx++)
	    		cout << '(' << ndx+1 << ") " << tests[ndx].subject << '\n';;
	    	cout << endl;
	    	return 0;
	    }
	    if (args.is_set("-t")) {
	    	int t_index = stol(args.get_value("-t"));
	    	for (ndx=0; tests[ndx].OK(); ndx++) {
	    		if (ndx+1 == t_index) {
	    			tests[ndx].test();
	    			break;
	    		}
	    	}
	    	if (!tests[ndx].OK()) {
	    		cerr << "Unknown test index\n";
	    		return -1;
	    	}
	    }
    } catch(const runtime_error& re) {
        cerr << "Unit test failure: " << re.what() << endl;
        args.usage();
        return 1;
    }
    cout << ">> test completed.\n";
    return 0;
}