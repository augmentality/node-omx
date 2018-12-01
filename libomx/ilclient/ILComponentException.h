#pragma once
#include <string>


class ILComponentException
{
    public:

        ILComponentException(const std::string errMsg): errorMessage(errMsg)
        {

        }

        std::string errorMessage;

};