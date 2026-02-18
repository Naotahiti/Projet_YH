// Fill out your copyright notice in the Description page of Project Settings.


#include "../AI/AI_Base.h"
#include "AIController.h"
#include "../AI/MyAIController.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BrainComponent.h"
#include "FlowField.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
AAI_Base::AAI_Base()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	//PrimaryActorTick.TickInterval = 0.1f;

	//AIControllerClass = AMyAIController::StaticClass();
	//AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

}

// Called when the game starts or when spawned
void AAI_Base::BeginPlay()
{
	Super::BeginPlay();

	AIC = Cast<AMyAIController>(GetController());

	player = UGameplayStatics::GetPlayerCharacter(GetWorld(),0);

	if (AIC)
	{
		blackboard = AIC->GetBlackboardComponent();
		
	}
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(),
		AFlowField::StaticClass(),
		Found
	);

	if (Found.Num() > 0)
	{
		Field = Cast<AFlowField>(Found[0]);
	}

}

// Called every frame
void AAI_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	distance = UKismetMathLibrary::Vector_Distance(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)->GetActorLocation(), GetActorLocation());
	
	//AController* c = GetController(); // AI is possessed or unpossessed according to the distance to avoid perf' issues
	if (AIC)
		blackboard->SetValueAsFloat("distance", distance);
	if (!Field)

	{
		GEngine->AddOnScreenDebugMessage(-1, 0.1, FColor::Green, "no field found");
			return;
	}

	
	Dir = Field->SampleFlow(GetActorLocation());
	FVector ToPlayer = player->GetActorLocation() - GetActorLocation();
	SetActorRotation(Dir.Rotation());
	
	AddActorWorldOffset(Dir*Speed*DeltaTime );
	
	
}

// Called to bind functionality to input
void AAI_Base::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

UBehaviorTree* AAI_Base::getbt()
{
	return tree;
}

void AAI_Base::IsSeenByPlayer(bool on)
{
	if (on)
		GetCharacterMovement()->DisableMovement();

	else
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
}

