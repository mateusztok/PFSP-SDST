#include "app/Application.hpp"
#include <iostream>
#include <vector>
#include <string>

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <data_file> [algorithms...]" << std::endl;
        return 1;
    }

    std::string dataFile = argv[1];
    std::vector<std::string> algArgs;
    
    // Zbieramy wszystkie argumenty po nazwie pliku (np. neh, simulated_annealing)
    for (int i = 2; i < argc; ++i) {
        algArgs.push_back(argv[i]);
    }

    Application app(dataFile, algArgs);
    return app.run();
}