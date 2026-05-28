#pragma once

UENUM()
enum class EAgentState : uint8
{
    Idle,
    Move,
    Attack
};

class AI_Base
{
public:

    FVector Position;
    FVector Velocity;
    int chunkID;
    EAgentState State;



    AI_Base(FVector InitPos, FVector InitVel)
        : Position(InitPos), Velocity(InitVel)
    {
    }





};