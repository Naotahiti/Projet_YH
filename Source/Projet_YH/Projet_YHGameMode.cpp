// Copyright Epic Games, Inc. All Rights Reserved.

#include "Projet_YHGameMode.h"
#include "Projet_YHCharacter.h"
#include "UObject/ConstructorHelpers.h"

AProjet_YHGameMode::AProjet_YHGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
