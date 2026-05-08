#pragma once
class AI_Base
{
public:

    FVector Position;
    FVector Velocity;
    int chunkID;



    AI_Base(FVector InitPos, FVector InitVel)
        : Position(InitPos), Velocity(InitVel)
    {
    }





};