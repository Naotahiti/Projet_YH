// Fill out your copyright notice in the Description page of Project Settings.


#include "../AI/MyAIController.h"
#include "AI_Base.h"

void AMyAIController::OnPossess(APawn* pawn)
{
	Super::OnPossess(pawn);

	AAI_Base* ai = Cast<AAI_Base>(pawn);
	RunBehaviorTree(ai->getbt());
}
