#include "app/Application.hpp"
#include <iostream>

int main(int argc, char **argv)
{

    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <data_file> <algorithm>" << std::endl;
        std::cerr << "Algorithms: quick_neh, simulated_annealing, adaptive_sa" << std::endl;
        std::cerr << "Combinations: quick_neh+sa, quick_neh+asa, quick_neh+sa+asa" << std::endl;
        return 1;
    }

    std::string dataFile = argv[1];
    std::string algorithm = argv[2];

    Application app(dataFile, algorithm);
    return app.run();
}