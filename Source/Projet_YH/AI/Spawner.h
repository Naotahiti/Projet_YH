// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../AI/AI_Base.h"
#include "Engine/TimerHandle.h"
#include "Spawner.generated.h"


//USTRUCT AIdata
//
//{
//
//}

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

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere ,Category = "Enemies to spawn")
	TArray<TSubclassOf<AAI_Base>> Enemies;

	UPROPERTY(EditAnywhere, Category = "Statistics") // rate per second
	float SpawnRate = 1.;

	UPROPERTY(EditAnywhere, Category = "Statistics")
	int NumToSpawn;

	UPROPERTY(EditAnywhere, Category = "Statistics")
	int MaxEnemiesAllowed;

	void SpawnEnemy();

	UPROPERTY(EditAnywhere ,Category = "Statistics")
	int MaxEnemiesAllowedPerType; // control the amount of enemies allowed accroding to their type 

	FTimerHandle Handle;

	UPROPERTY(EditAnywhere, Category = "Statistics")
	TSubclassOf<AAI_Base> AIclass;

	TArray<TSubclassOf<AAI_Base>> CurrentAIs; // store all enemies 

	FTimerHandle spawnhandle;

	UInstancedStaticMeshComponent* ism; // AIs are ism or charcters depending on the distance from the player , for better perf'
	
	void upLOD(); // promote AI to higher LOD = AI gets more complex

	void downLOD(); // retrograde AI to lower LOD = AI gets less complex

};
