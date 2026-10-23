#include "init.hpp"

#include <string_view>

int main(int argc, char* argv[]) {
    bool verticalSync = true;
    for (int i = 1; i < argc; ++i) {
        if (std::string_view(argv[i]) == "--uncapped") {
            verticalSync = false;
        }
    }
    return Engine::run(false, verticalSync) ? 0 : 1;
}
