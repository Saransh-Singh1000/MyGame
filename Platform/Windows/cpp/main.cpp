#include <iostream>

#include "Core.hpp"

//This is the main entry point.

int main(){
    Log(LogLevel::Info, "Hello, World!" );
    Log(LogLevel::Warn, "Hello World");
    Log(LogLevel::Error, "Hello World");
    Log(LogLevel::Fatal, "Hello World");
    return 0;
}