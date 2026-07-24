// Fill out your copyright notice in the Description page of Project Settings.


#include "../AI/Explosive.h"
#include "../AI/Spawner.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
AExplosive::AExplosive()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AExplosive::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(th, this, &AExplosive::explode, 0.2, true, -1.); // instead of tick , to gain performance

	
}

// Called every frame
void AExplosive::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AExplosive::explode()
{
	ASpawner* Spawner = Cast<ASpawner>(UGameplayStatics::GetActorOfClass(GetWorld(), ASpawner::StaticClass()));

	if (!Spawner) return;

	const FVector MyPos = GetActorLocation();
	const float TriggerRadiusSq = 200.f * 200.f;

	for (FHordeAISystem::FChunk& C : Spawner->AI.Chunks)
		for (int32 i = 0; i < C.Count; i++)
		{
			if (FVector::DistSquared(C.Positions[i], MyPos) < TriggerRadiusSq)
			{
				GetWorldTimerManager().ClearTimer(th);

				
				Spawner->DamageInRadius(GetActorLocation(), 500.f, 100.f);
				Destroy();
				return;
			}
		}
}

