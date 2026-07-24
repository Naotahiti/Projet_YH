#include "FlowField.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Async/Async.h"           // AsyncTask
#include "HAL/ThreadManager.h"


AFlowField::AFlowField()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.1f; // 10 Hz suffit pour le flow field
}

// ─────────────────────────────────────────────
void AFlowField::BeginPlay()
{
    Super::BeginPlay();

    PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (!PlayerPawn) return;

    const int32 Total = GridSizeX * GridSizeY;

    Cells.Init(Total);
    CellsBack.Init(Total);
    BFSQueue.Reserve(Total);

    BakeObstacles();
    Cells.Blocked = bBlockedBaked;
    CellsBack.Blocked = bBlockedBaked;

    LastPlayerPos = PlayerPawn->GetActorLocation();
    FloodFillAsync(LastPlayerPos, GetActorLocation());
    Swap(Cells, CellsBack);
}

// to see thje cells before running the project , 
void AFlowField::OnConstruction(const FTransform& Transform)
{
    if (!cell_center_visualizer) return;

    for (int32 Y = 0; Y < GridSizeY; ++Y)
        for (int32 X = 0; X < GridSizeX; ++X)
            DrawDebugPoint(GetWorld(), GetCellCenter(X, Y), 10.f, FColor::Red, false, 10.f);
}

void AFlowField::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (!PlayerPawn) return;

    
    if (bPendingSwap.load(std::memory_order_acquire))
    {
        // Le swap se fait sur le Game Thread : aucune race possible car
        // le worker ne touche plus à CellsBack une fois bPendingSwap = true.
        Swap(Cells, CellsBack);
        bPendingSwap.store(false, std::memory_order_release);
    }

    // only recalculate when player has moved , for better performances due to lesser calculations
    const FVector CurrentPos = PlayerPawn->GetActorLocation();
    const bool    bMoved = FVector::DistSquared(CurrentPos, LastPlayerPos)
        > FMath::Square(DistanceMinForRecalculation);

    if (bMoved && !bComputeInProgress.load(std::memory_order_relaxed))
    {
        LastPlayerPos = CurrentPos;
        RequestFloodFillAsync(CurrentPos, GetActorLocation());
    }
}


void AFlowField::RequestFloodFillAsync(FVector TargetPos, FVector GridOrigin)
{

   
    bComputeInProgress.store(true, std::memory_order_release);

   

    AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask,
        [this, TargetPos, GridOrigin]()
        {
            FloodFillAsync(TargetPos, GridOrigin);

            // Signal au Game Thread : d'abord bPendingSwap, PUIS on libère bComputeInProgress
            bPendingSwap.store(true, std::memory_order_release);
            bComputeInProgress.store(false, std::memory_order_release);
        });
}


void AFlowField::FloodFillAsync(FVector TargetPos, FVector GridOrigin)
{
    static const FIntPoint kNeighbors[8] = {
      { 1, 0}, {-1, 0}, { 0, 1}, { 0,-1},
      { 1, 1}, { 1,-1}, {-1, 1}, {-1,-1}
    };

    const int32 Total = GridSizeX * GridSizeY;

    
    CellsBack.ResetForFlood(Total);

   
    const FVector Local = TargetPos - GridOrigin;

    const int32 TargetX = FMath::Clamp(
        FMath::FloorToInt(Local.X / CellSize), 0, GridSizeX - 1);
    const int32 TargetY = FMath::Clamp(
        FMath::FloorToInt(Local.Y / CellSize), 0, GridSizeY - 1);

    const int32 TargetIndex = GetCellIndex(TargetX, TargetY);
    CellsBack.Costs[TargetIndex] = 0.f;

    
    BFSQueue.Reset(); 
    BFSQueue.Add(FIntPoint(TargetX, TargetY));
    int32 Head = 0;

    while (Head < BFSQueue.Num())
    {
        const FIntPoint Cell = BFSQueue[Head++];
        const int32 CellIdx = GetCellIndex(Cell.X, Cell.Y);
        const float CellCost = CellsBack.Costs[CellIdx];

   
        const FVector CellCenter = GridOrigin
            + FVector(Cell.X * CellSize + CellSize * 0.5f,
                Cell.Y * CellSize + CellSize * 0.5f, 0.f);

        for (const FIntPoint& N : kNeighbors)
        {
            const int32 NX = Cell.X + N.X;
            const int32 NY = Cell.Y + N.Y;

            // Bounds check
            if ((uint32)NX >= (uint32)GridSizeX || (uint32)NY >= (uint32)GridSizeY)
                continue;

            const int32 NIdx = GetCellIndex(NX, NY);

            if (CellsBack.Blocked[NIdx]) continue;

            // no diagonals if blocked
            const bool bDiagonal = (N.X != 0 && N.Y != 0);
            if (bDiagonal)
            {
                if (CellsBack.Blocked[GetCellIndex(Cell.X + N.X, Cell.Y)] ||
                    CellsBack.Blocked[GetCellIndex(Cell.X, Cell.Y + N.Y)])
                    continue;
            }

            
            const float MoveCost = bDiagonal ? CellSize * 1.41421356f : CellSize;
            const float NewCost = CellCost + MoveCost;

            if (NewCost < CellsBack.Costs[NIdx])
            {
                CellsBack.Costs[NIdx] = NewCost;

                // Direction 
                const FVector NeighborCenter = GridOrigin
                    + FVector(NX * CellSize + CellSize * 0.5f,
                        NY * CellSize + CellSize * 0.5f, 0.f);

                CellsBack.Directions[NIdx] = (CellCenter - NeighborCenter).GetSafeNormal();

                BFSQueue.Add(FIntPoint(NX, NY)); // maj du chemin vers le joueur
            }
        }
    }
}

//lecture de directions
FVector AFlowField::SampleFlow(const FVector& WorldPosition) const
{
    if (Cells.Directions.Num() == 0) return FVector::ZeroVector;

    const FVector Local = WorldPosition - GetActorLocation();

    const int32 X = FMath::Clamp(FMath::FloorToInt(Local.X / CellSize), 0, GridSizeX - 1);
    const int32 Y = FMath::Clamp(FMath::FloorToInt(Local.Y / CellSize), 0, GridSizeY - 1);

    return Cells.Directions[GetCellIndex(X, Y)];
}

// prise en compte des obstacles , au begin play  car statiques
void AFlowField::BakeObstacles()
{
    const int32 Total = GridSizeX * GridSizeY;
    bBlockedBaked.SetNumZeroed(Total);

    for (int32 Y = 0; Y < GridSizeY; ++Y)
        for (int32 X = 0; X < GridSizeX; ++X)
            bBlockedBaked[GetCellIndex(X, Y)] = IsCellBlocked(X, Y);
}

// ─────────────────────────────────────────────
bool AFlowField::IsCellBlocked(int32 X, int32 Y) const
{
    const FVector Center = GetCellCenter(X, Y);
    const FVector Start = Center + FVector(0.f, 0.f, 5000.f);
    const FVector End = Center + FVector(0.f, 0.f, 50.f);

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    // Simple trace verticale pour détecter les obstacles au sol
    const bool bHit = GetWorld()->LineTraceSingleByChannel(
        Hit, Start, Center, ECC_WorldStatic, Params);
    if (bHit)
        return Hit.GetActor()->ActorHasTag("Obstacle");
    else 
        return false;
    
}