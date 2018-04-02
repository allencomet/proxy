#include "cctest.h"
#include "../sys.h"
#include "../string_util.h"

DEF_uint32(timeout, -1, "timeout in ms for test cases");
DEF_bool(a, false, "run all tests if true");

namespace cctest {

void init_cctest(int argc, char** argv) {
    std::vector<std::string> args = ccflag::init_ccflag(argc, argv);
    cclog::init_cclog(*argv);

    Tests::instance()->set_args(args);
}

void run_tests() {
    Tests::instance()->run_tests();
}

const std::vector<std::string>& args() {
    return Tests::instance()->get_args();
}

void Tests::run_tests() {
    if (FLG_a) {//run all tests if true
        for (uint32 i = 0; i < _tests.size(); ++i) {
            _runner.run_test(_tests[i]);
        }

    } else {//default run the tests if on
        for (uint32 i = 0; i < _tests.size(); ++i) {
            if (_tests[i]->on()) {
                _runner.run_test(_tests[i]);
            } else {
                delete _tests[i];
                _tests[i] = NULL;
            }
        }
    }
}

void TestRunner::run_test(Test* test) {
    _test.reset(test);
    _ev.reset();

    set_lightblue();
    CERR << ">>> begin " << test->name() << " test";
    reset_color();

    _thread.reset(new safe::Thread(boost::bind(&TestRunner::thread_fun, this)));
    _thread->start();

    sys::timer t;
    std::pair<std::string, uint32> res;

    if (!_ev.timed_wait(FLG_timeout)) {
        set_lightblue();
        CERR << "<<< " << test->name() << " test timeout: " << t.ms() << "ms\n";
        reset_color();

        _thread->cancel();

    } else {
        set_lightblue();
        CERR << "<<< " << test->name() << " test done: " << t.ms() << "ms\n";
        reset_color();

        _thread->join();
    }
}

} // namespace cctest
