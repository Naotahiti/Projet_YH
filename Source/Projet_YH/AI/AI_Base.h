// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
//#include "FlowField.h"
#include "AI_Base.generated.h"
 
class UBehaviorTree;
class UBrainComponent;
class AFlowField;

// for animation blueprint
UENUM(BlueprintType)
enum class EAIState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Patrol UMETA(DisplayName = "Walk"),
	Chase UMETA(DisplayName = "Shoot"),
	Attack UMETA(DisplayName = "Melee"),
	Flee UMETA(DisplayName = "Flee"),
	GetDamage UMETA(DisplayName = "Damaged"),
	Death UMETA(DisplayName = "Death")
};

UCLASS()
class PROJET_YH_API AAI_Base : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AAI_Base();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere , Category = "Statistics")
	float Health;

	UPROPERTY(EditAnywhere, Category = "Statistics")
	float Damage;

	UPROPERTY(EditAnywhere, Category = "Statistics")
	int RateOfAttack;

	UPROPERTY(EditAnywhere, Category = "Statistics")
	float Range;

	UPROPERTY(EditAnywhere, Category = "Statistics")
	float Speed = 200.;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	EAIState CurrentState;

	UPROPERTY(EditAnywhere, Category = "Behavior tree")
	UBehaviorTree* tree;

	UFUNCTION(BlueprintCallable)
	UBehaviorTree* getbt();

	UPROPERTY(BlueprintReadOnly)
	FVector Dir;

	UBlackboardComponent* blackboard;

	ACharacter* player;

	// activate / deactivate AI's movement
	void IsSeenByPlayer(bool on);

	double distance; // to player

	AAIController* AIC;


	AFlowField* Field;

};
