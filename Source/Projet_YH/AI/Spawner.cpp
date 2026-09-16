// Fill out your copyright notice in the Description page of Project Settings.


#include "../AI/Spawner.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "../AI/AI_Base.h"
#include "../AI/FlowField.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "AnimToTextureInstancePlaybackHelpers.h"
#include "../Projet_YHCharacter.h"

// Sets default values
ASpawner::ASpawner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	//RootComponent =CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	ISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ISM"));
	ISM->SetupAttachment(RootComponent);
	ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ISM->bDisallowNanite = true;
	ISM->NumCustomDataFloats = 1;
}

// Called when the game starts or when spawned
void ASpawner::BeginPlay()
{
	Super::BeginPlay();

	
	Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

	AI.Scale = Scale;
	AI.Reserve(NumToSpawn);
	//Batch.SetNum(NumToSpawn);
	AI.SepRadiusSq = SeparationRadius;
	AI.NeighborRadiusSq = NeighbourRadius;
	AI.SepWeight = SeparationWeight;
	AI.AliWeight = AlignemntWeight;
	AI.CohWeight = CohesionWeight;
	


	for (int32 i = 0; i < NumToSpawn; i++)
	{
		SpawnEnemy();

	}
	HC.Origin = GetActorLocation() - FVector(5000.f, 5000.f, 0.f);
	HC.CellSize = 200.f;
	HC.SizeX = 50;
	HC.SizeY = 50;
	HC.Bake(GetWorld(), HC.Origin);
	Batch.SetNum(ISM->GetInstanceCount());
	for (FTransform& T : Batch)
		T.SetScale3D(Scale);
	SetActorTickEnabled(true);
	//GetWorldTimerManager().SetTimer(handledebug, this, &ASpawner::debugg, 3, true, -1.);
}



void ASpawner::debugg()
{
	//GEngine->AddOnScreenDebugMessage(1, 10, FColor::Red, FString::FromInt(AllAIs.Num()), true);
	GEngine->AddOnScreenDebugMessage(2, 10, FColor::Red, FString::FromInt(ISM->GetInstanceCount()), true);
}



void ASpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//if (Batch.Num() == 0 || AI.TotalEntities() == 0) return;
	if (!Player || !FF) return;

	FrameCounter++;
	const FVector PlayerPos = Player->GetActorLocation();

	
	AI.RunLOD(PlayerPos); // 2 LOD so far , might be removed cause not needed so far
	
	AI.RunMovement(FF, Speed, DeltaTime, [this](const FVector& Pos)
		{
			return HC.Sample(Pos);
		});
	AI.RunGravity(DeltaTime, [this](const FVector& Pos)
		{return HC.Sample(Pos); });
	
	AI.RunRender(Batch, ISM);


}



void ASpawner::SpawnEnemy()
{

	const int32 rdx = FMath::RandRange(-1000., 1000.);
	const int32 rdy = FMath::RandRange(-1000., 1000.);
	FVector SpawnLoc = GetActorLocation() + FVector(rdx, rdy, 500.f);

	// to spawn on the ground
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit, SpawnLoc, SpawnLoc - FVector(0.f, 0.f, 5000.f),
		ECC_WorldStatic, Params
	);

	if (bHit) SpawnLoc.Z = Hit.ImpactPoint.Z ;

	FTransform Tr;
	Tr.SetLocation(SpawnLoc);
	Tr.SetScale3D(Scale);
	const int32 ISMIndex = ISM->AddInstance(Tr, true);
	
	AI.AddEntity(SpawnLoc, ISMIndex, 0., 1., 1., 100.);
}
