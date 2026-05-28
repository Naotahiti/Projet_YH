// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projet_YH/Projet_YHCharacter.h"
#include "../AI/AI_Base.h"
#include "FlowField.generated.h"



USTRUCT(BlueprintType)
struct PROJET_YH_API FFlowCell
{
    GENERATED_BODY()

    UPROPERTY()
    float Cost = TNumericLimits<float>::Max();

    UPROPERTY()
    bool bBlocked = false;

    UPROPERTY()
    FVector Direction = FVector::ZeroVector;
};

UCLASS()
class PROJET_YH_API AFlowField : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
public:
    AFlowField();

protected:
    virtual void BeginPlay() override;

    virtual void OnConstruction(
        const FTransform& Transform
    ) override;

    UPROPERTY(EditAnywhere,Category = "custom debug")
    bool cell_center_visualizer;

public:
    virtual void Tick(float DeltaTime) override;

    
    void GenerateFlowField();
    void FloodFill();

    FVector SampleFlow(const FVector& WorldPosition) const;

   
    int32 GetCellIndex(int32 X, int32 Y) const;
    FVector GetCellCenter(int32 X, int32 Y) const;
    bool IsCellBlocked(int32 X, int32 Y) const;

    TArray<AActor*> actorstoignore;

protected:
 
    UPROPERTY(EditAnywhere, Category = "Flow Field")
    int32 GridSizeX = 100;

    UPROPERTY(EditAnywhere, Category = "Flow Field")
    int32 GridSizeY = 100;

    UPROPERTY(EditAnywhere, Category = "Flow Field")
    float CellSize = 100.f;

    UPROPERTY(EditAnywhere, Category = "Flow Field")
    float DistanceMinForRecalculation = 200.f;

    UPROPERTY()
    TArray<FFlowCell> Cells;


    AI_Base* ai;

    APawn* PlayerPawn;

    FVector LastPlayerPos;
};
