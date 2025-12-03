// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../AI/AI_Base.h"
#include "Flying_AI.generated.h"

/**
 * 
 */
UCLASS()
class PROJET_YH_API AFlying_AI : public AAI_Base
{
	GENERATED_BODY()

public : 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "pathfinding")
	float NodeSpacing = 200.0f;
	
};
