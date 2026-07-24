// Fill out your copyright notice in the Description page of Project Settings.

#include "../AI/testsU.h"
#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "../AI/AI_Base.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestSpawn,
    "HordeIA.Spawn.AjouterEntite",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

    bool FTestSpawn::RunTest(const FString& Parameters)
{
    FHordeAISystem AI;
    AI.Reserve(10);

    // check if spawn at the right location
    for (int32 i = 0; i < 5; i++)
        AI.AddEntity(FVector(i * 100.f, 0.f, 0.f), i, 0.f, 1.f, 1.f);

    TestEqual("Nombre d'entités après spawn", AI.TotalEntities(), 5);
    TestEqual("Position X entité 0", AI.Chunks[0].Positions[0].X, 0.0);
    TestEqual("Position X entité 1", AI.Chunks[0].Positions[1].X, 100.0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestChunks,
    "HordeIA.Chunks.CreationAutomatique",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

    bool FTestChunks::RunTest(const FString& Parameters)
{
    FHordeAISystem AI;
    AI.Reserve(256);

    // must create 2 chunks if 129 AIs
    for (int32 i = 0; i < 129; i++)
        AI.AddEntity(FVector::ZeroVector, i, 0.f, 1.f, 1.f);

    TestEqual("Nombre de chunks pour 129 entités", AI.Chunks.Num(), 2);
    TestEqual("Count du chunk 0", AI.Chunks[0].Count, 128);
    TestEqual("Count du chunk 1", AI.Chunks[1].Count, 1);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestLOD,
    "HordeIA.LOD.NiveauxParDistance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

    bool FTestLOD::RunTest(const FString& Parameters)
{
    FHordeAISystem AI;
    AI.Reserve(3);

    // Proche = LOD 0
    AI.AddEntity(FVector(0.f, 0.f, 0.f), 0, 0.f, 1.f, 1.f);
    // Moyenne distance = LOD 1
    AI.AddEntity(FVector(1000.f, 0.f, 0.f), 1, 0.f, 1.f, 1.f);
    // Loin = LOD 2
    AI.AddEntity(FVector(5000.f, 0.f, 0.f), 2, 0.f, 1.f, 1.f);

    AI.RunLOD(FVector::ZeroVector);

    TestEqual("Entité proche = LOD 0", (int32)AI.Chunks[0].LODLevel[0], 0);
    TestEqual("Entité moyenne = LOD 1", (int32)AI.Chunks[0].LODLevel[1], 1);
    TestEqual("Entité loin = LOD 2", (int32)AI.Chunks[0].LODLevel[2], 2);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestDegats,
    "HordeIA.Combat.DegatsEnRayon",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

    bool FTestDegats::RunTest(const FString& Parameters)
{
    FHordeAISystem AI;
    AI.Reserve(3);

    AI.AddEntity(FVector(0.f, 0.f, 0.f), 0, 0.f, 1.f, 1.f, 100.f); // dans le rayon
    AI.AddEntity(FVector(50.f, 0.f, 0.f), 1, 0.f, 1.f, 1.f, 100.f); // dans le rayon
    AI.AddEntity(FVector(5000.f, 0.f, 0.f), 2, 0.f, 1.f, 1.f, 100.f); // hors rayon

    AI.DamageEntitiesInRadius(FVector::ZeroVector, 200.f, 50.f);

    TestEqual("Entité 0 HP après dégâts", AI.Chunks[0].HP[0], 50.f);
    TestEqual("Entité 1 HP après dégâts", AI.Chunks[0].HP[1], 50.f);
    TestEqual("Entité 2 HP inchangée", AI.Chunks[0].HP[2], 100.f);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestDegatsFatals,
    "HordeIA.Combat.DegatsFatals",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

    bool FTestDegatsFatals::RunTest(const FString& Parameters)
{
    FHordeAISystem AI;
    AI.Reserve(1);

    AI.AddEntity(FVector::ZeroVector, 0, 0.f, 1.f, 1.f, 30.f);
    AI.DamageEntitiesInRadius(FVector::ZeroVector, 100.f, 50.f);

    // HP <= 0 = RunDeath s'occupera de supprimer l'IA
    TestTrue("HP <= 0 après dégâts fatals", AI.Chunks[0].HP[0] <= 0.f);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestDegatsHorsRayon,
    "HordeIA.Combat.PasDeDegatsHorsRayon",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

    bool FTestDegatsHorsRayon::RunTest(const FString& Parameters)
{
    FHordeAISystem AI;
    AI.Reserve(1);

    AI.AddEntity(FVector(1000.f, 0.f, 0.f), 0, 0.f, 1.f, 1.f, 100.f);
    AI.DamageEntitiesInRadius(FVector::ZeroVector, 50.f, 50.f);

    TestEqual("HP inchangé hors rayon", AI.Chunks[0].HP[0], 100.f);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestReserve,
    "HordeIA.Memoire.ReserveSansResize",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

    bool FTestReserve::RunTest(const FString& Parameters)
{
    FHordeAISystem AI;
    AI.Reserve(1000);

    for (int32 i = 0; i < 1000; i++)
        AI.AddEntity(FVector(i * 10.f, 0.f, 0.f), i, 0.f, 1.f, 1.f);

    TestEqual("1000 entités spawned", AI.TotalEntities(), 1000);
    TestEqual("Nombre de chunks", AI.Chunks.Num(), 8); // 1000/128 = 7.8 qui devient 8

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestHPInitiaux,
    "HordeIA.Spawn.HPInitiaux",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

    bool FTestHPInitiaux::RunTest(const FString& Parameters)
{
    FHordeAISystem AI;
    AI.Reserve(2);

    AI.AddEntity(FVector::ZeroVector, 0, 0.f, 1.f, 1.f, 75.f);
    AI.AddEntity(FVector::ZeroVector, 1, 0.f, 1.f, 1.f, 100.f);

    TestEqual("HP entité 0 = 75", AI.Chunks[0].HP[0], 75.f);
    TestEqual("HP entité 1 = 100", AI.Chunks[0].HP[1], 100.f);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestTotalApresMort,
    "HordeIA.Combat.TotalEntitesDecroit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

    bool FTestTotalApresMort::RunTest(const FString& Parameters)
{
    FHordeAISystem AI;
    AI.Reserve(5);

    for (int32 i = 0; i < 5; i++)
        AI.AddEntity(FVector(i * 10.f, 0.f, 0.f), i, 0.f, 1.f, 1.f, 100.f);

    TestEqual("5 entités avant dégâts", AI.TotalEntities(), 5);

    // Met les HP à 0 manuellement pour simuler une mort
    AI.Chunks[0].HP[0] = -1.f;
    AI.Chunks[0].HP[1] = -1.f;

    // Compte les entités vivantes manuellement
    int32 Vivants = 0;
    for (const FHordeAISystem::FChunk& C : AI.Chunks)
        for (int32 i = 0; i < C.Count; i++)
            if (C.HP[i] > 0.f) Vivants++;

    TestEqual("3 entités vivantes après 2 morts", Vivants, 3);

    return true;
};