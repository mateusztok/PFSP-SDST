#pragma once
#include <string>
#include "Controller.hpp"
#include <iostream>

class Application
{
private:
    std::string instancePath;
    std::string algorithmName;

public:
    Application(const std::string &dataFile, const std::string &algorithm)
        : instancePath(dataFile), algorithmName(algorithm) {}

    int run()
    {
        std::cout << "Data file: " << instancePath << std::endl;
        std::cout << "Algorithm: " << algorithmName << std::endl;

        try
        {
            Controller controller(instancePath, algorithmName);
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
