// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "MyAIController.generated.h"

/**
 * 
 */
UCLASS()
class PROJET_YH_API AMyAIController : public AAIController
{
	GENERATED_BODY()

public : 

	//explicit  AMyAIController(FObjectInitializer const& obj);

	//UPROPERTY(EditAnywhere , Category = "Tree")
	//UBehaviorTree* tree;

	virtual void OnPossess(APawn* pawn)override;

	
	
};
