#pragma once
#include <string>

class FFException
{
    public:

        explicit FFException(const std::string errMsg): errorMessage(errMsg)
        {

        }

        std::string errorMessage;
};