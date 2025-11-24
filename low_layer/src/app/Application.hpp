#pragma once
#include <string>
#include "Controller.hpp"
#include <iostream>

class Application
{
private:
    std::string instancePath;
    std::vector<std::string> algorithms;

public:
    Application(const std::string &dataFile, const std::vector<std::string> &algs)
        : instancePath(dataFile), algorithms(algs) {}

    int run()
    {
        std::cout << "Data file: " << instancePath << std::endl;
        std::cout << "Algorithms: ";
        for (size_t i = 0; i < algorithms.size(); ++i)
        {
            if (i)
                std::cout << ",";
            std::cout << algorithms[i];
        }
        std::cout << std::endl;

        try
        {
            Controller controller(instancePath, algorithms);
            controller.execute();
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }

        return 0;
    }
};
