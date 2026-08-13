#include "init/init.hpp"

int main(int argc, char* argv[]) {
    // Hand over control to Common/src/init/init.cpp
    if (!Engine::run()) {
        return -1;
    }
    return 0;
}