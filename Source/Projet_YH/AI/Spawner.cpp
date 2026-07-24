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

	
	AI.RunLOD(PlayerPos);
	
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

//void ASpawner::updateAI(int index , float d)
//{
//	if (!AllAIs.IsValidIndex(index) || !AllAIs[index]) return;
//	AI_Base* A = AllAIs[index];
//
//	if (!AllAIs[index]->bOnGround)
//	{
//		// Simple snap vers la hauteur bakée dans FHeightGrid
//		const float BakedZ = HeightGrid.Sample(AllAIs[index]->Position);
//		AllAIs[index]->Position.Z = FMath::FInterpTo(
//			AllAIs[index]->Position.Z, BakedZ, d, 15.f);
//
//		if (FMath::Abs(AllAIs[index]->Position.Z - BakedZ) < 2.f)
//			AllAIs[index]->bOnGround = true;
//	}
//
//		const FVector FlowDir = ff->SampleFlow(A->Position);
//
//
//		A->Position.X += FlowDir.X * speed * d;
//		A->Position.Y += FlowDir.Y * speed * d;
//	
//	BatchTransforms[index].SetLocation(A->Position);
//	BatchTransforms[index].SetRotation(A->Rotation.Quaternion());
//	BatchTransforms[index].SetScale3D(scaleAI);
//}

//void ASpawner::updateAI(int index, float d)
//{
//	AI_Base* A = AllAIs[index];
//
//	double t0 = FPlatformTime::Seconds();
//
//	// Ligne 1 : FallVelocity
//	A->FallVelocity -= 980.f * d;
//
//	double t1 = FPlatformTime::Seconds();
//
//	// Ligne 2 : CollisionQueryParams
//	FCollisionQueryParams Params;
//	Params.AddIgnoredActor(this);
//
//	double t2 = FPlatformTime::Seconds();
//
//	// Ligne 3 : SampleFlow
//	const FVector FlowDir = ff->SampleFlow(A->Position);
//
//	double t3 = FPlatformTime::Seconds();
//
//	// Ligne 4 : déplacement
//	A->Position.X += FlowDir.X * speed * d;
//	A->Position.Y += FlowDir.Y * speed * d;
//
//	double t4 = FPlatformTime::Seconds();
//
//	// Ligne 5 : BatchTransforms
//	BatchTransforms[index].SetLocation(A->Position);
//	BatchTransforms[index].SetRotation(A->Rotation.Quaternion());
//	BatchTransforms[index].SetScale3D(scaleAI);
//
//	double t5 = FPlatformTime::Seconds();
//
//	// Affiche seulement pour l'agent 0 (évite le spam)
//	if (index == 0)
//	{
//		GEngine->AddOnScreenDebugMessage(20, 0.f, FColor::White,
//			FString::Printf(TEXT("FallVel   : %.4f ms"), (t1 - t0) * 1000.0));
//		GEngine->AddOnScreenDebugMessage(21, 0.f, FColor::White,
//			FString::Printf(TEXT("Params    : %.4f ms"), (t2 - t1) * 1000.0));
//		GEngine->AddOnScreenDebugMessage(22, 0.f, FColor::White,
//			FString::Printf(TEXT("SampleFlow: %.4f ms"), (t3 - t2) * 1000.0));
//		GEngine->AddOnScreenDebugMessage(23, 0.f, FColor::White,
//			FString::Printf(TEXT("Move      : %.4f ms"), (t4 - t3) * 1000.0));
//		GEngine->AddOnScreenDebugMessage(24, 0.f, FColor::White,
//			FString::Printf(TEXT("Transforms: %.4f ms"), (t5 - t4) * 1000.0));
//	}
//}


//FVector ASpawner::separate(int32 index)
//{
//	FVector Force = FVector::ZeroVector;
//	const FVector MyPos = AllAIs[index]->Position;
//
//	for (int32 j : Grid.GetNeighbors(MyPos))
//	{
//		if (j == index) continue;
//		const float Dist = FVector::DistSquared(MyPos, AllAIs[j]->Position);
//		if (Dist > 0.f && Dist < DistanceMin)
//			Force += (MyPos - AllAIs[j]->Position).GetSafeNormal();
//	}
//	return Force.GetSafeNormal();
//
//}
//
//FVector ASpawner::align(int32 index)
//{
//	FVector AvgVel = FVector::ZeroVector;
//	int32   Count = 0;
//	const FVector MyPos = AllAIs[index]->Position;
//
//	for (int32 j : Grid.GetNeighbors(MyPos))
//	{
//		if (j == index) continue;
//		if (FVector::DistSquared(MyPos, AllAIs[j]->Position) < 500.f)
//		{
//			AvgVel += AllAIs[j]->Velocity;
//			Count++;
//		}
//	}
//	if (Count == 0) return FVector::ZeroVector;
//	return (AvgVel / Count).GetSafeNormal();
//}
//
//FVector ASpawner::cohesion(int32 index)
//{
//	FVector Sum = FVector::ZeroVector;
//	int32   Count = 0;
//	const FVector MyPos = AllAIs[index]->Position;
//
//	for (int32 j : Grid.GetNeighbors(MyPos))
//	{
//		if (j == index) continue;
//		const float Dist = FVector::DistSquared(MyPos, AllAIs[j]->Position);
//		if (Dist > DistanceMin && Dist < 1000.f)
//		{
//			Sum += AllAIs[j]->Position;
//			Count++;
//		}
//	}
//	if (Count == 0) return FVector::ZeroVector;
//	return ((Sum / Count) - MyPos).GetSafeNormal();
//
//}
//
//void ASpawner::UpdateAgentCollision(int index, float DeltaTime)
//{
//	UWorld* World = GetWorld();
//
//	// Position future
//	FVector NextPos = AllAIs[index]->Position + AllAIs[index]->Velocity * DeltaTime;
//
//	// Paramtres du trace — ignorer les autres agents
//	FCollisionQueryParams Params;
//	Params.AddIgnoredActor(this);
//
//	
//	FHitResult WallHit;
//	FVector MoveDir = AllAIs[index]->Velocity.GetSafeNormal2D();
//
//	if (World->SweepSingleByChannel(
//		WallHit,
//		AllAIs[index]->Position,
//		AllAIs[index]->Position + MoveDir * 40.f, // 40u = rayon de la capsule
//		FQuat::Identity,
//		ECC_WorldStatic,
//		FCollisionShape::MakeSphere(40.f),
//		Params))
//	{
//		// Glisser le long du mur
//		FVector Normal2D = WallHit.ImpactNormal;
//		Normal2D.Z = 0.f;
//		Normal2D.Normalize();
//
//		// Projeter la vlocit sur le plan du mur
//		AllAIs[index]->Velocity = FVector::VectorPlaneProject(
//			AllAIs[index]->Velocity,
//			Normal2D
//		);
//	}
//	else
//	{
//		AllAIs[index]->Position = NextPos;
//	}
//}
//
//
//void ASpawner::AlignAgentToGround(int index , float d)
//{
//	AI_Base* A = AllAIs[index];
//	if (A->GroundNormal.IsNearlyZero()) return;
//
//	const FVector Forward = A->Velocity.GetSafeNormal2D();
//	if (Forward.IsNearlyZero()) return;
//
//	const FVector Right = FVector::CrossProduct(A->GroundNormal, Forward);
//	const FVector UpAligned = FVector::CrossProduct(Forward, Right);
//	const FRotator Target = FRotationMatrix::MakeFromXZ(Forward, UpAligned).Rotator();
//
//	A->Rotation = FMath::RInterpTo(A->Rotation, Target, d, 10.f);
//}

//void ASpawner::RequestGroundTraceAsync(int32 index)
//{
//	if (!AllAIs.IsValidIndex(index)) return;
//
//	FVector Start = AllAIs[index]->Position - FVector(0, 0, 100.f);
//	FVector End = AllAIs[index]->Position - FVector(0, 0, 2000.f);
//
//	// Crer un wrapper qui vit jusqu'au callback
//	FPendingTrace* Pending = new FPendingTrace();
//	Pending->AgentIndex = index;
//	ActiveDelegates.Add(Pending);
//
//	Pending->Delegate.BindLambda(
//		[this, Pending](const FTraceHandle& Handle, FTraceDatum& Data)
//		{
//			const int32 AgentIdx = Pending->AgentIndex;
//
//			if (AllAIs.IsValidIndex(AgentIdx) && AllAIs[AgentIdx])
//			{
//				if (Data.OutHits.Num() > 0)
//				{
//					AllAIs[AgentIdx]->Position.Z = Data.OutHits[0].ImpactPoint.Z;
//					AllAIs[AgentIdx]->GroundNormal = Data.OutHits[0].ImpactNormal;
//					AllAIs[AgentIdx]->bOnGround = true;
//					AllAIs[AgentIdx]->FallVelocity = 0.f;
//				}
//				else
//				{
//					AllAIs[AgentIdx]->bOnGround = false;
//				}
//			}
//
//			// Nettoyer
//			ActiveDelegates.Remove(Pending);
//			delete Pending;
//		}
//	);
//
//	GetWorld()->AsyncLineTraceByChannel(
//		EAsyncTraceType::Single,
//		Start, End,
//		ECC_WorldStatic,
//		FCollisionQueryParams::DefaultQueryParam,
//		FCollisionResponseParams::DefaultResponseParam,
//		&Pending->Delegate
//	);
//}



//float ASpawner::FHeightGrid::Sample(const FVector& Pos) const
//{
//	
//		const FVector Local = Pos - Origin;
//
//		const int32 X =
//			FMath::Clamp(
//				FMath::FloorToInt(Local.X / CellSize),
//				0,
//				Width - 1);
//
//		const int32 Y =
//			FMath::Clamp(
//				FMath::FloorToInt(Local.Y / CellSize),
//				0,
//				Height - 1);
//
//		return Heights[GetIndex(X, Y)];
//
//}
