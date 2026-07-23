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

// ─────────────────────────────────────────────
void AFlowField::OnConstruction(const FTransform& Transform)
{
    if (!cell_center_visualizer) return;

    for (int32 Y = 0; Y < GridSizeY; ++Y)
        for (int32 X = 0; X < GridSizeX; ++X)
            DrawDebugPoint(GetWorld(), GetCellCenter(X, Y), 10.f, FColor::Red, false, 10.f);
}

// ─────────────────────────────────────────────
void AFlowField::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (!PlayerPawn) return;

    // ── Swap si le worker a terminé ──────────────────────────────────
    // bPendingSwap est mis à true par le worker AVANT de passer bComputeInProgress à false,
    // donc on est sûrs que CellsBack est entièrement écrit quand on arrive ici.
    if (bPendingSwap.load(std::memory_order_acquire))
    {
        // Le swap se fait sur le Game Thread : aucune race possible car
        // le worker ne touche plus à CellsBack une fois bPendingSwap = true.
        Swap(Cells, CellsBack);
        bPendingSwap.store(false, std::memory_order_release);
    }

    // ── Lance un nouveau calcul si le joueur a bougé ──────────────────
    const FVector CurrentPos = PlayerPawn->GetActorLocation();
    const bool    bMoved = FVector::DistSquared(CurrentPos, LastPlayerPos)
        > FMath::Square(DistanceMinForRecalculation);

    if (bMoved && !bComputeInProgress.load(std::memory_order_relaxed))
    {
        LastPlayerPos = CurrentPos;
        RequestFloodFillAsync(CurrentPos, GetActorLocation());
    }
}

// ─────────────────────────────────────────────
//  Lance le calcul sur un thread background
// ─────────────────────────────────────────────
void AFlowField::RequestFloodFillAsync(FVector TargetPos, FVector GridOrigin)
{

    // Marque le début du calcul
    bComputeInProgress.store(true, std::memory_order_release);

    // Copie de bBlockedBaked dans CellsBack.Blocked (la référence au tableau
    // principal ne doit pas être lue depuis le worker sans garantie de vie)
    // → déjà fait au BeginPlay et à chaque BakeObstacles(); on ne retouche pas Blocked ici.

    AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask,
        [this, TargetPos, GridOrigin]()
        {
            FloodFillAsync(TargetPos, GridOrigin);

            // Signal au Game Thread : d'abord bPendingSwap, PUIS on libère bComputeInProgress
            bPendingSwap.store(true, std::memory_order_release);
            bComputeInProgress.store(false, std::memory_order_release);
        });
}

// ─────────────────────────────────────────────
//  BFS — s'exécute sur un worker thread
//  N'accède JAMAIS à Cells, seulement CellsBack
// ─────────────────────────────────────────────
void AFlowField::FloodFillAsync(FVector TargetPos, FVector GridOrigin)
{
    static const FIntPoint kNeighbors[8] = {
      { 1, 0}, {-1, 0}, { 0, 1}, { 0,-1},
      { 1, 1}, { 1,-1}, {-1, 1}, {-1,-1}
    };

    const int32 Total = GridSizeX * GridSizeY;

    // ── Reset du buffer back ──────────────────────────────────────────
    // Blocked est déjà bon (bakée au BeginPlay, jamais modifiée)
    CellsBack.ResetForFlood(Total);

    // ── Cellule cible ─────────────────────────────────────────────────
    const FVector Local = TargetPos - GridOrigin;

    const int32 TargetX = FMath::Clamp(
        FMath::FloorToInt(Local.X / CellSize), 0, GridSizeX - 1);
    const int32 TargetY = FMath::Clamp(
        FMath::FloorToInt(Local.Y / CellSize), 0, GridSizeY - 1);

    const int32 TargetIndex = GetCellIndex(TargetX, TargetY);
    CellsBack.Costs[TargetIndex] = 0.f;

    // ── BFS avec TArray + curseur (zéro allocation) ───────────────────
    BFSQueue.Reset(); // reset le Num() sans libérer la mémoire
    BFSQueue.Add(FIntPoint(TargetX, TargetY));
    int32 Head = 0;

    while (Head < BFSQueue.Num())
    {
        const FIntPoint Cell = BFSQueue[Head++];
        const int32 CellIdx = GetCellIndex(Cell.X, Cell.Y);
        const float CellCost = CellsBack.Costs[CellIdx];

        // Centre de la cellule courante — calculé une fois pour les 8 voisins
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

            // Coupe-coin : un diagonal ne passe pas si les deux cases adjacentes sont bloquées
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

                // Direction : du voisin vers la cellule courante (vers le joueur)
                const FVector NeighborCenter = GridOrigin
                    + FVector(NX * CellSize + CellSize * 0.5f,
                        NY * CellSize + CellSize * 0.5f, 0.f);

                CellsBack.Directions[NIdx] = (CellCenter - NeighborCenter).GetSafeNormal();

                BFSQueue.Add(FIntPoint(NX, NY));
            }
        }
    }
}

// ─────────────────────────────────────────────
//  SampleFlow — appelé depuis le Spawner (Game Thread)
//  Lit uniquement Cells.Directions[] : hot path, SoA friendly
// ─────────────────────────────────────────────
FVector AFlowField::SampleFlow(const FVector& WorldPosition) const
{
    if (Cells.Directions.Num() == 0) return FVector::ZeroVector;

    const FVector Local = WorldPosition - GetActorLocation();

    const int32 X = FMath::Clamp(FMath::FloorToInt(Local.X / CellSize), 0, GridSizeX - 1);
    const int32 Y = FMath::Clamp(FMath::FloorToInt(Local.Y / CellSize), 0, GridSizeY - 1);

    return Cells.Directions[GetCellIndex(X, Y)];
}

// ─────────────────────────────────────────────
//  BakeObstacles — appelé UNE seule fois au BeginPlay
//  Les traces sont synchrones mais acceptables ici (chargement)
// ─────────────────────────────────────────────
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
    const FVector Start = Center + FVector(0.f, 0.f, 150.f);
    const FVector End = Center + FVector(0.f, 0.f, 50.f);

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    // Simple trace verticale pour détecter les obstacles au sol
    const bool bHit = GetWorld()->LineTraceSingleByChannel(
        Hit, Start, End, ECC_WorldStatic, Params);

    // Tu peux affiner ici : bHit && Hit.GetActor()->ActorHasTag("Obstacle")
    return bHit;
}