#pragma once
#include <string>

class ILClientException
{
    public:

        ILClientException(const std::string errMsg): errorMessage(errMsg)
        {

        }

        std::string errorMessage;

};