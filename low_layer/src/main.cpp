#include "app/Application.hpp"
#include <iostream>

int main(int argc, char **argv)
{

    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <data_file> <algorithm>" << std::endl;
        std::cerr << "Algorithms: neh, simulated_annealing" << std::endl;
        std::cerr << "Combinations: neh+simulated_annealing" << std::endl;
        return 1;
    }

    std::string dataFile = argv[1];

    std::vector<std::string> algArgs;
    for (int i = 2; i < argc; ++i)
    {
        algArgs.push_back(argv[i]);
    }

    Application app(dataFile, algArgs);
    return app.run();
}