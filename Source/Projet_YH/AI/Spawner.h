// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../AI/AI_Base.h"
//#include "../AI/FlowField.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/TimerHandle.h"
#include "AnimToTextureDataAsset.h"
#include "Spawner.generated.h"


class AFlowField;

UCLASS()
class PROJET_YH_API ASpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASpawner();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	//virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	//virtual void OnConstruction(
	//	const FTransform& Transform
	//) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category = "Statistics AI")
	bool spawnonconstruction = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite ,Category = "Statistics AI")
	UAnimToTextureDataAsset* DA;

	UPROPERTY(EditAnywhere, Category = "Statistics AI")
	float Speed = 200.;

	UPROPERTY(EditAnywhere, Category = "Statistics AI")
	FVector Scale = FVector::OneVector; 

	UPROPERTY(EditAnywhere, Category = "Statistics") // rate per second
	float SpawnRate = 1.;

	UPROPERTY(EditAnywhere, Category = "Statistics")
	int NumToSpawn;

	UPROPERTY(EditAnywhere, Category = "Boids forces")
	float SeparationRadius = 200.;

	UPROPERTY(EditAnywhere, Category = "Boids forces")
	float NeighbourRadius = 1000.; 

	UPROPERTY(EditAnywhere, Category = "Boids forces")
	float SeparationWeight = 2.; 

	UPROPERTY(EditAnywhere, Category = "Boids forces")
	float AlignemntWeight = 2.;

	UPROPERTY(EditAnywhere, Category = "Boids forces")
	float CohesionWeight = 2.;

	int32 FrameCounter = 0;

	struct FMyHeightCache
	{
		TArray<float> Heights;
		FVector       Origin;
		float         CellSize = 200.f;
		int32         SizeX = 50;
		int32         SizeY = 50;

		void Bake(UWorld* World, FVector InOrigin)
		{
			Origin = InOrigin;
			Heights.SetNum(SizeX * SizeY);

			for (int32 Y = 0; Y < SizeY; Y++)
				for (int32 X = 0; X < SizeX; X++)
				{
					const FVector Start = Origin + FVector(X * CellSize, Y * CellSize, 5000.f);
					const FVector End = Start - FVector(0, 0, 10000.f);

					FHitResult Hit;
					FCollisionQueryParams Params;
					Heights[Y * SizeX + X] = World->LineTraceSingleByChannel(
						Hit, Start, End, ECC_WorldStatic, Params)
						? Hit.ImpactPoint.Z
						: Origin.Z;
				}
		}

		float Sample(const FVector& Pos) const
		{
			if (Heights.Num() == 0) return 0.f;


			const int32 X = FMath::Clamp(
				FMath::FloorToInt((Pos.X - Origin.X) / CellSize), 0, SizeX - 1);
			const int32 Y = FMath::Clamp(
				FMath::FloorToInt((Pos.Y - Origin.Y) / CellSize), 0, SizeY - 1);
			return Heights[Y * SizeX + X];
		}
	};

	FMyHeightCache HC;

	
	

	
		struct FHeightGrid
	{
		

		UPROPERTY()
		int32 Width = 0;

		UPROPERTY()
		int32 Height = 0;

		UPROPERTY()
		float CellSize = 100.f;

		UPROPERTY()
		FVector Origin;

		UPROPERTY()
		TArray<float> Heights;

		FORCEINLINE int32 GetIndex(int32 X, int32 Y) const
		{
			return Y * Width + X;
		}

		//float Sample(const FVector& Pos) const;
	};

	UPROPERTY(EditAnywhere)
	int32 GridWidth = 200;

	UPROPERTY(EditAnywhere)
	int32 GridHeight = 200;

	UPROPERTY(EditAnywhere)
	float HeightCellSize = 100.f;

	FHeightGrid HeightGrid;

	struct FPendingTrace
	{
		int32 AgentIndex;
		FTraceDelegate Delegate;
	};

	TArray<FPendingTrace*> ActiveDelegates;

	UFUNCTION(BlueprintCallable)
	void SpawnEnemy();

	UPROPERTY(EditAnywhere)
	AFlowField* FF = nullptr;

	APawn* Player;
	TArray<FTransform> Batch;
	FHordeAISystem AI;

	FTimerHandle handledebug;

	void debugg();

	FTimerHandle Handle;

	void DamageInRadius(FVector HitPos, float Radius, float Damage)
	{
		AI.DamageEntitiesInRadius(HitPos, Radius, Damage);
		AI.RunDeath(ISM);              
		Batch.SetNum(ISM->GetInstanceCount());
		
	}

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Horde") // pour HUD
	int32 GetEnemyCount() const
	{
		return AI.TotalEntities();
	}
	
	// if we want to use a timer to spawn
	FTimerHandle spawnhandle;

	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Statistics")
	UInstancedStaticMeshComponent* ISM; // AIs are ism or charcters depending on the distance from the player , for better perf'
	
	

};
