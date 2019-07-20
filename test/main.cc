#include "../util/util.h"

int main(int argc, char **argv) {
    ccflag::init_ccflag(argc, argv);
    cclog::init_cclog(argv[0]);

    cctest::init_cctest(argc, argv);
    cctest::run_tests();
}