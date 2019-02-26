#pragma once

enum class ControlQueueCommandType
{
    None = 0,
    Load = 1,
    Play = 2,
    Pause = 3,
    Stop = 4,
    SetSpeed = 5,
    SetLoop = 6
};