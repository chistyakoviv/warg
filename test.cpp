#include <cassert>
#include <string>
#include "warg.h"

int main(int argc, char** argv) {
    {
        // It tests help message
        warg::ArgPack pack;
        warg::parse(argc, argv, pack);
        std::string expected = "Usage: ./warg [-h]\n\nOptions:\n -h - show usage info\n";
        std::string result = pack.showHelp(argv[0]);
        assert(result == expected && "help message test failed");
    }
    std::cout << "All tests passed!" << std::endl;
}