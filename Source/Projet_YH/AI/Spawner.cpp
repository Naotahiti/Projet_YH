// Fill out your copyright notice in the Description page of Project Settings.


#include "../AI/Spawner.h"
#include "Kismet/GameplayStatics.h"
#include "../Projet_YHCharacter.h"

// Sets default values
ASpawner::ASpawner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ASpawner::BeginPlay()
{
	Super::BeginPlay();

	
	
}

// Called every frame
void ASpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ASpawner::SpawnEnemy()
{
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    FVector SpawnLocation;
    int MaxAttempts = 50;

    for (int i = 0; i < MaxAttempts; i++)
    {
        // Génère une position aléatoire autour du joueur
        float Distance = FMath::RandRange(1000.f, 2000.f);
        float Angle = FMath::RandRange(0.f, 360.f);

        ACharacter* player = Cast<AProjet_YHCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
        SpawnLocation = player->GetActorLocation() + FVector(
            FMath::Cos(FMath::DegreesToRadians(Angle)) * Distance,
            FMath::Sin(FMath::DegreesToRadians(Angle)) * Distance,
            0.f
        );

        // Vérifie si hors du champ de vision
        if (!IsLocationInView(SpawnLocation, PC))
        {
            GetWorld()->SpawnActor<AAI_Base>(AI, SpawnLocation, FRotator::ZeroRotator);
            break;
        }
    }
}

bool ASpawner::IsLocationInView(FVector Location, APlayerController* PlayerC)
{
    if (!PlayerC) return false;

    FVector CameraLocation;
    FRotator CameraRotation;
    PlayerC->GetPlayerViewPoint(CameraLocation, CameraRotation);

    
    FVector ViewDir = CameraRotation.Vector();
    FVector ToLocation = (Location - CameraLocation).GetSafeNormal();

    float DotProduct = FVector::DotProduct(ViewDir, ToLocation);

    // Si le dot product est négatif, c'est derrière la caméra
    return DotProduct > 0.1f; // Ajustez selon vos besoins
}




