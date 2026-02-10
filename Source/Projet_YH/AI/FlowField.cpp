// Fill out your copyright notice in the Description page of Project Settings.


#include "../AI/FlowField.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Containers/Queue.h"

// Sets default values
AFlowField::AFlowField()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AFlowField::BeginPlay()
{
    Super::BeginPlay();

    PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (!PlayerPawn) return;

    Cells.SetNum(GridSizeX * GridSizeY);

    LastPlayerPos = PlayerPawn->GetActorLocation();
    GenerateFlowField();
}

void AFlowField::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!PlayerPawn) return;

    
    if (FVector::Dist(PlayerPawn->GetActorLocation(), LastPlayerPos) > 200.f)
    {
        GenerateFlowField();
        LastPlayerPos = PlayerPawn->GetActorLocation();
    }
}

void AFlowField::GenerateFlowField()
{
    // Init des cellules
    for (int32 Y = 0; Y < GridSizeY; ++Y)
    {
        for (int32 X = 0; X < GridSizeX; ++X)
        {
            int32 Index = GetCellIndex(X, Y);
            Cells[Index].bBlocked = IsCellBlocked(X, Y);
            Cells[Index].Cost = TNumericLimits<float>::Max();
            Cells[Index].Direction = FVector::ZeroVector;
        }
    }

    FloodFill();
}

void AFlowField::FloodFill()
{
    if (!PlayerPawn) return;

    //GEngine->AddOnScreenDebugMessage(-1, 10., FColor::Cyan, "aaaaaaaaaaaaaaaa");

    FVector TargetPos = PlayerPawn->GetActorLocation();

    int32 TargetX = FMath::Clamp(
        FMath::FloorToInt((TargetPos.X - GetActorLocation().X) / CellSize),
        0, GridSizeX - 1
    );

    int32 TargetY = FMath::Clamp(
        FMath::FloorToInt((TargetPos.Y - GetActorLocation().Y) / CellSize),
        0, GridSizeY - 1
    );

    TQueue<FIntPoint> Queue;

    int32 TargetIndex = GetCellIndex(TargetX, TargetY);
    Cells[TargetIndex].Cost = 0.f;
    Queue.Enqueue(FIntPoint(TargetX, TargetY));

    const TArray<FIntPoint> Neighbors = {
        {1,0},{-1,0},{0,1},{0,-1}
    };

    while (!Queue.IsEmpty())
    {
        FIntPoint Cell;
        Queue.Dequeue(Cell);

        int32 Index = GetCellIndex(Cell.X, Cell.Y);
        float CellCost = Cells[Index].Cost;

        for (const FIntPoint& N : Neighbors)
        {
            int32 NX = Cell.X + N.X;
            int32 NY = Cell.Y + N.Y;

            if (NX < 0 || NX >= GridSizeX || NY < 0 || NY >= GridSizeY)
                continue;

            int32 NIndex = GetCellIndex(NX, NY);
            if (Cells[NIndex].bBlocked)
                continue;

            float NewCost = CellCost + CellSize;

            if (NewCost < Cells[NIndex].Cost)
            {
                Cells[NIndex].Cost = NewCost;

                FVector From = GetCellCenter(NX, NY);
                FVector To = GetCellCenter(Cell.X, Cell.Y);
                Cells[NIndex].Direction = (To - From).GetSafeNormal();

                Queue.Enqueue(FIntPoint(NX, NY));
            }
        }
    }
}

FVector AFlowField::SampleFlow(const FVector& WorldPosition) const
{
    FVector LocalPos = WorldPosition - GetActorLocation();

    int32 X = FMath::Clamp(
        FMath::FloorToInt(LocalPos.X / CellSize),
        0, GridSizeX - 1
    );

    int32 Y = FMath::Clamp(
        FMath::FloorToInt(LocalPos.Y / CellSize),
        0, GridSizeY - 1
    );

    int32 Index = GetCellIndex(X, Y);



    return Cells[Index].Direction;
}

bool AFlowField::IsCellBlocked(int32 X, int32 Y) const
{
    FVector Center = GetCellCenter(X, Y);

    FHitResult Hit;
    FVector Start = Center + FVector(0, 0, 50.f);
    FVector End = Center + FVector(0, 0, 300.f);

    FCollisionQueryParams Params;
    Params.bReturnPhysicalMaterial = false;
    Params.AddIgnoredActor(this);

    bool bHit = GetWorld()->LineTraceSingleByChannel(
        Hit,
        Start,
        End,
        ECC_WorldStatic,
        Params
    );

    if (bHit && Hit.GetActor())
    {
        return Hit.GetActor()->ActorHasTag("Obstacle");
    }

    return false;
}

int32 AFlowField::GetCellIndex(int32 X, int32 Y) const
{
    return Y * GridSizeX + X;
}

FVector AFlowField::GetCellCenter(int32 X, int32 Y) const
{
    return GetActorLocation()
        + FVector(X * CellSize + CellSize * 0.5f,
            Y * CellSize + CellSize * 0.5f,
            0.f);
}


