#include <exception>
#include <iostream>

int runCoreTests();

int main()
{
    try {
        return runCoreTests();
    } catch (const std::exception& e) {
        std::cerr << "Unhandled test exception: " << e.what() << '\n';
        return 1;
    }
}
