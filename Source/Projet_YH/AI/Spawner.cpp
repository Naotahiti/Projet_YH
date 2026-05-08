// Fill out your copyright notice in the Description page of Project Settings.


#include "../AI/Spawner.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "../AI/AI_Base.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "../Projet_YHCharacter.h"

// Sets default values
ASpawner::ASpawner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	ism = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ism"));
	for (int i = 0; i < NumToSpawn; i++)
	{
		SpawnEnemy();
		
	}
	
}

// Called when the game starts or when spawned
void ASpawner::BeginPlay()
{
	Super::BeginPlay();

	if(spawnoutofscreen)
	GetWorldTimerManager().SetTimer(spawnhandle,this,&ASpawner::SpawnEnemy,SpawnRate,true,-1.);
}

// Called every frame
void ASpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
void ASpawner::SpawnEnemy()
{
	int rdx = FMath::RandRange(-1000, 1000);
	int rdy = FMath::RandRange(-1000, 1000);
	FVector rdl = GetActorLocation() + FVector(rdx, rdy, 0.);

	AI_Base* NewAI = new AI_Base(rdl, FVector(0., 0., 0.));
			AllAIs.Add(NewAI);
			FTransform tr;
			tr.SetLocation(rdl);
			ism->AddInstance(tr);
			ism->MarkRenderStateDirty();
}

//void ASpawner::SpawnEnemy()
//{
//	//GEngine->AddOnScreenDebugMessage(1, 10., FColor::Cyan, "ffffffrrr", true);
//	FVector2D Size = UWidgetLayoutLibrary::GetViewportSize(GetWorld());
//
//	float rdX = FMath::FRandRange(0.,Size.X);
//	float rdY = FMath::FRandRange(0.,Size.Y);
//
//	FVector2D rdtop = FVector2D(rdX,0);
//
//	FVector WorldLocation;
//	FVector WorldDirection;
//
//	bool o = UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(GetWorld(), 0), rdtop,WorldLocation,	
//		WorldDirection);
//
//	FVector dir = WorldLocation + WorldDirection * 2000.;
//
//	FCollisionQueryParams Params;
//	Params.AddIgnoredActor(this);
//
//	FHitResult hit;
//	bool bhit = GetWorld()->LineTraceSingleByChannel(hit, WorldLocation, dir , ECC_Visibility , Params);
//	
//	
//
//	if (bhit)
//	{
//		FVector spawnloc = hit.Location;
//		UWorld* World = GetWorld();
//		if (!World) return;
//
//		//UKismetSystemLibrary::DrawDebugLine(GetWorld(), WorldLocation,spawnloc,FColor::Red , 10. , 5. );
//
//		FActorSpawnParameters Paramsp;
//		Paramsp.SpawnCollisionHandlingOverride =
//			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
//
//		FVector Location(0.f, 0.f, 100.f);
//		FRotator Rotation = FRotator::ZeroRotator;
//
//		FTransform tr;
//		AI_Base* NewAI = new AI_Base(spawnloc + Location, FVector(0.,0.,0.));
//		AllAIs.Add(NewAI);
//		tr.SetLocation(spawnloc + Location);
//		ism->AddInstance(tr);
//
//		
//	}
//}

void ASpawner::upLOD()
{
}


void ASpawner::downLOD()
{
}




