#ifndef INIT_HPP
#define INIT_HPP

namespace Engine {
    // Initializes SDL, opens a window, runs the main event loop, and cleans up.
    bool run(bool useOpenGL = false, bool verticalSync = true);
}

#endif // INIT_HPP