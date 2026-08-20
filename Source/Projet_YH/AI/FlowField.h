#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include <atomic>
#include "FlowField.generated.h"

// ─────────────────────────────────────────────
//  SoA : données séparées par champ pour minimiser
//  la cache pressure lors du SampleFlow (lecture seule)
// ─────────────────────────────────────────────
struct FFlowFieldData
{
    TArray<FVector> Directions; // hot  — lu à chaque SampleFlow()
    TArray<float>   Costs;      // warm — lu/écrit pendant FloodFill
    TArray<bool>    Blocked;    // cold — bakée une seule fois

    void Init(int32 Total)
    {
        Directions.SetNumZeroed(Total);
        Costs.SetNum(Total);
        Blocked.SetNumZeroed(Total);
    }

    void ResetForFlood(int32 Total)
    {
        for (int32 i = 0; i < Total; i++)
        {
            Costs[i] = TNumericLimits<float>::Max();
            Directions[i] = FVector::ZeroVector;
        }
    }
};

UCLASS()
class AFlowField : public AActor
{
    GENERATED_BODY()

public:
    AFlowField();

    // ── Paramètres exposés ──
    UPROPERTY(EditAnywhere, Category = "My Custom variables | Grid")   int32   GridSizeX = 50;
    UPROPERTY(EditAnywhere, Category = "My Custom variables | Grid")   int32   GridSizeY = 50;
    UPROPERTY(EditAnywhere, Category = "My Custom variables | Grid")   float   CellSize = 200.f;
    UPROPERTY(EditAnywhere, Category = "My Custom variables | Grid")   float   DistanceMinForRecalculation = 100.f;
    UPROPERTY(EditAnywhere, Category = "My Custom variables | Debug")  bool    cell_center_visualizer = false;

    // ── API publique (appelée depuis Spawner, thread-safe en lecture) ──
    FVector SampleFlow(const FVector& WorldPosition) const;

protected:
    virtual void BeginPlay()  override;
    virtual void Tick(float DeltaTime) override;
    virtual void OnConstruction(const FTransform& Transform) override;

private:
    // ── Double buffer ──
    //   Cells lu par les agents (Game Thread uniquement)
    //   CellsBack écrit par le worker thread
    FFlowFieldData Cells;
    FFlowFieldData CellsBack;

    // Obstacles baked
    TArray<bool> bBlockedBaked;

    // ── Synchronisation / opti
    std::atomic<bool> bComputeInProgress{ false };
    std::atomic<bool> bPendingSwap{ false };

    // ── BFS réutilisable (zéro alloc par recalcul) ──
    TArray<FIntPoint> BFSQueue;

    // 
    APawn* PlayerPawn = nullptr;
    FVector LastPlayerPos;

    // functions for obstacle
    void BakeObstacles();                                        
    bool IsCellBlocked(int32 X, int32 Y) const;

    
    void RequestFloodFillAsync(FVector TargetPos, FVector GridOrigin);

    
    void FloodFillAsync(FVector TargetPos, FVector GridOrigin);

    // Helpers
    FORCEINLINE int32   GetCellIndex(int32 X, int32 Y) const { return Y * GridSizeX + X; }
    FORCEINLINE FVector GetCellCenter(int32 X, int32 Y) const
    {
        return GetActorLocation()
            + FVector(X * CellSize + CellSize * 0.5f,
                Y * CellSize + CellSize * 0.5f, 0.f);
    }
};