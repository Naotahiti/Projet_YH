#pragma once
#include "CoreMinimal.h"
#include "../AI/FlowField.h"
#include "Components/InstancedStaticMeshComponent.h"


class FHordeAISystem
{
public:

    static constexpr int32 CHUNK_SIZE = 128;

    struct FChunk
    {
        FVector  Positions[CHUNK_SIZE];
        FVector  Velocities[CHUNK_SIZE];
        FQuat    Rotations[CHUNK_SIZE];
        float    FallVelocity[CHUNK_SIZE];
        bool     OnGround[CHUNK_SIZE];
        FVector  GroundNormal[CHUNK_SIZE];

        float    HP[CHUNK_SIZE];

        float    AnimTime[CHUNK_SIZE];
        float    AnimStart[CHUNK_SIZE];
        float    AnimEnd[CHUNK_SIZE];
        float    AnimRate[CHUNK_SIZE];

        int32    ISMIndex[CHUNK_SIZE];

        uint8    LODLevel[CHUNK_SIZE];
        float    DistSq[CHUNK_SIZE];

        int32    Count = 0;

        int32 SeqStart = 0;

        void InitAt(int32 i, const FVector& Pos, int32 InISMIndex,
            float InAnimStart, float InAnimEnd, float InAnimRate,
            float InHP = 100.f)
        {
            Positions[i] = Pos;
            Velocities[i] = FVector::ZeroVector;
            Rotations[i] = FQuat::Identity;
            FallVelocity[i] = 0.f;
            OnGround[i] = false;
            GroundNormal[i] = FVector::UpVector;

            HP[i] = InHP;

            AnimTime[i] = FMath::RandRange(InAnimStart, InAnimEnd);
            AnimStart[i] = InAnimStart;
            AnimEnd[i] = InAnimEnd;
            AnimRate[i] = InAnimRate;

            ISMIndex[i] = InISMIndex;

            LODLevel[i] = 0;
            DistSq[i] = 0.f;
        }
    };

    TArray<FChunk> Chunks;

    struct FSpatialGrid
    {
        struct FEntry { FIntPoint Cell; int32 GlobalIndex; };

        TArray<FEntry>    Entries;
        TArray<FIntPoint> CellKeys;
        TArray<int32>     CellStarts;
        float             CellSize = 500.f;

        void Reserve(int32 MaxAgents)
        {
            Entries.Reserve(MaxAgents);
            CellKeys.Reserve(MaxAgents);
            CellStarts.Reserve(MaxAgents);
        }

        void Clear()
        {
            Entries.Reset();
            CellKeys.Reset();
            CellStarts.Reset();
        }

        void Insert(int32 GlobalIndex, const FVector& Pos)
        {
            Entries.Add({ GetCell(Pos), GlobalIndex });
        }

        void Build()
        {
            if (Entries.Num() == 0) return;

            Entries.Sort([](const FEntry& A, const FEntry& B)
                {
                    const int64 KeyA = ((int64)A.Cell.X << 32) | (uint32)A.Cell.Y;
                    const int64 KeyB = ((int64)B.Cell.X << 32) | (uint32)B.Cell.Y;
                    return KeyA < KeyB;
                });

            CellKeys.Reset();
            CellStarts.Reset();

            FIntPoint LastCell = Entries[0].Cell - FIntPoint(1, 0);
            for (int32 i = 0; i < Entries.Num(); i++)
            {
                if (Entries[i].Cell != LastCell)
                {
                    CellKeys.Add(Entries[i].Cell);
                    CellStarts.Add(i);
                    LastCell = Entries[i].Cell;
                }
            }
            CellStarts.Add(Entries.Num());
        }

        template<typename TFunc>
        void ForEachNeighbor(const FVector& Pos, TFunc&& Callback) const
        {
            const FIntPoint Center = GetCell(Pos);

            for (int32 dx = -1; dx <= 1; dx++)
                for (int32 dy = -1; dy <= 1; dy++)
                {
                    const FIntPoint QueryCell = Center + FIntPoint(dx, dy);
                    const int32 Found = BinarySearchCell(QueryCell);
                    if (Found == INDEX_NONE) continue;

                    const int32 Start = CellStarts[Found];
                    const int32 End = CellStarts[Found + 1];
                    for (int32 i = Start; i < End; i++)
                        Callback(Entries[i].GlobalIndex);
                }
        }

    private:
        FORCEINLINE FIntPoint GetCell(const FVector& Pos) const
        {
            return FIntPoint(
                FMath::FloorToInt(Pos.X / CellSize),
                FMath::FloorToInt(Pos.Y / CellSize));
        }

        int32 BinarySearchCell(const FIntPoint& Key) const
        {
            int32 Lo = 0, Hi = CellKeys.Num() - 1;
            while (Lo <= Hi)
            {
                const int32 Mid = (Lo + Hi) / 2;
                const FIntPoint& M = CellKeys[Mid];
                if (M.X < Key.X || (M.X == Key.X && M.Y < Key.Y)) Lo = Mid + 1;
                else if (M.X > Key.X || (M.X == Key.X && M.Y > Key.Y)) Hi = Mid - 1;
                else    return Mid;
            }
            return INDEX_NONE;
        }
    };

    FSpatialGrid   Grid;

    FVector Scale;
    float SepRadiusSq = 10000.f;
    float NeighborRadiusSq = 40000.f;
    float SepWeight = 1.5f; // separation weight
    float AliWeight = 1.0f; // alignment weight
    float CohWeight = 0.8f; // cohesion weight

    void Reserve(int32 TotalEntities)
    {
        Chunks.Reserve(TotalEntities / CHUNK_SIZE + 1);
        Grid.Reserve(TotalEntities);
    }

    TPair<int32, int32> AddEntity(const FVector& Pos, int32 ISMIndex,
        float AnimStart, float AnimEnd,
        float AnimRate, float HP = 100.f)
    {
        if (Chunks.Num() == 0 || Chunks.Last().Count == CHUNK_SIZE)
            Chunks.AddDefaulted();

        FChunk& C = Chunks.Last();
        const int32 SlotIdx = C.Count++;
        C.InitAt(SlotIdx, Pos, ISMIndex, AnimStart, AnimEnd, AnimRate, HP);

        return { Chunks.Num() - 1, SlotIdx };
    }

    int32 TotalEntities() const
    {
        int32 Total = 0;
        for (const FChunk& C : Chunks) Total += C.Count;
        return Total;
    }

    FORCEINLINE int32 ToGlobalIndex(int32 ci, int32 i) const
    {
        return ci * CHUNK_SIZE + i;
    }

    FORCEINLINE void FromGlobalIndex(int32 Global, int32& OutCI, int32& OutI) const
    {
        OutCI = Global / CHUNK_SIZE;
        OutI = Global % CHUNK_SIZE;
    }

    void RunLOD(const FVector& PlayerPos)
    {
        ParallelFor(Chunks.Num(), [&](int32 ci)
            {
                FChunk& C = Chunks[ci];
                for (int32 i = 0; i < C.Count; i++)
                {
                    C.DistSq[i] = FVector::DistSquared(C.Positions[i], PlayerPos);
                    C.LODLevel[i] =
                        C.DistSq[i] < 300000.f ? 0 :
                        C.DistSq[i] < 2000000.f ? 1 : 2;
                }
            });
    }

    void RunMovement(const AFlowField* FF, float Speed, float DeltaTime, TFunctionRef<float(const FVector&)> SampleHeight)
    {
        if (!FF) return;

        const int32 Total = TotalEntities();
        if (Total == 0) return;

        {
            int32 Seq = 0;
            for (int32 ci = 0; ci < Chunks.Num(); ci++)
            {
                Chunks[ci].SeqStart = Seq;
                Seq += Chunks[ci].Count;
            }
        }

        Grid.Clear();
        for (int32 ci = 0; ci < Chunks.Num(); ci++)
            for (int32 i = 0; i < Chunks[ci].Count; i++)
                Grid.Insert(Chunks[ci].SeqStart + i, Chunks[ci].Positions[i]);
        Grid.Build();

        TArray<FVector> NewVelocities;
        NewVelocities.SetNum(Total);
        //lecture
        ParallelFor(Chunks.Num(), [&](int32 ci)
            {
                FChunk& C = Chunks[ci];
                for (int32 i = 0; i < C.Count; i++)
                {
                    const int32 Seq = C.SeqStart + i;

                    if (C.LODLevel[i] == 2)
                    {
                        NewVelocities[Seq] = FVector::ZeroVector;
                        continue;
                    }

                    const FVector FlowDir = FF->SampleFlow(C.Positions[i]);
                    FVector DesiredVel(FlowDir.X * Speed, FlowDir.Y * Speed, 0.f);

                    if (C.LODLevel[i] == 0)
                    {
                        FVector Sep = FVector::ZeroVector;
                        FVector Ali = FVector::ZeroVector;
                        FVector Coh = FVector::ZeroVector;
                        int32   Count = 0;

                        const FVector MyPos = C.Positions[i];
                        const int32 MySeqIdx = Seq;

                        Grid.ForEachNeighbor(MyPos, [&](int32 NSeqIdx)
                            {
                                if (NSeqIdx == MySeqIdx) return;
                                int32 nci = 0, ni = NSeqIdx;
                                for (; nci < Chunks.Num(); nci++)
                                {
                                    if (ni < Chunks[nci].Count) break;
                                    ni -= Chunks[nci].Count;
                                }
                                if (!Chunks.IsValidIndex(nci)) return;

                                const FVector NPos = Chunks[nci].Positions[ni];
                                const float DistSq = FVector::DistSquared(MyPos, NPos);

                                if (DistSq > 0.f && DistSq < SepRadiusSq)
                                    Sep += (MyPos - NPos).GetSafeNormal();

                                if (DistSq < NeighborRadiusSq)
                                {
                                    Ali += Chunks[nci].Velocities[ni];
                                    Coh += NPos;
                                    Count++;
                                }
                            });

                        if (Count > 0)
                        {
                            Ali = (Ali / Count).GetSafeNormal();
                            Coh = ((Coh / Count) - MyPos).GetSafeNormal();
                        }

                        const FVector Steering = Sep * SepWeight
                            + Ali * AliWeight
                            + Coh * CohWeight;
                        DesiredVel.X += Steering.X * Speed;
                        DesiredVel.Y += Steering.Y * Speed;
                    }

                    const float VelSq = DesiredVel.SizeSquared2D();
                    if (VelSq > Speed * Speed * 4.f)
                        DesiredVel = DesiredVel.GetSafeNormal2D() * Speed * 2.f;

                    NewVelocities[Seq] = DesiredVel;
                }
            });

        // application
        for (int32 ci = 0; ci < Chunks.Num(); ci++)
        {
            FChunk& C = Chunks[ci];
            for (int32 i = 0; i < C.Count; i++)
            {
                const FVector& Vel = NewVelocities[C.SeqStart + i];
                C.Velocities[i] = Vel;
                C.Positions[i] += Vel * DeltaTime;
                C.Positions[i].Z = SampleHeight(C.Positions[i]) + 2.f;

                const FVector Dir2D(Vel.X, Vel.Y, 0.f);
                if (!Dir2D.IsNearlyZero())
                    C.Rotations[i] = Dir2D.ToOrientationQuat();
            }
        }

    }



    void RunGravity(float DeltaTime,
        TFunctionRef<float(const FVector&)> SampleHeight)
    {
        static constexpr float Gravity = 980.f;
        static constexpr float SnapDist = 5.f;

        for (FChunk& C : Chunks)
            for (int32 i = 0; i < C.Count; i++)
            {
               // if (C.LODLevel[i] != 0) continue;

                const float GroundZ = SampleHeight(C.Positions[i]);

                if (C.Positions[i].Z > GroundZ + SnapDist)
                {

                    C.FallVelocity[i] -= Gravity * DeltaTime;
                    C.Positions[i].Z += C.FallVelocity[i] * DeltaTime;
                    C.OnGround[i] = false;
                }
                else
                {

                    C.Positions[i].Z = GroundZ;
                    C.FallVelocity[i] = 0.f;
                    C.OnGround[i] = true;
                }
            }
    }

   

    //// avoid looping on UpdateInstanceTransform
    void RunRender(TArray<FTransform>& Batch, UInstancedStaticMeshComponent* ISM)
    {
        if (!ISM) return;
        
       
        ParallelFor(Chunks.Num(), [&](int32 ci)
            {
                FChunk& C = Chunks[ci];
                for (int32 i = 0; i < C.Count; i++)
                {
                    if (!Batch.IsValidIndex(C.ISMIndex[i])) continue;
                    Batch[C.ISMIndex[i]].SetLocation(C.Positions[i]);
                    Batch[C.ISMIndex[i]].SetRotation(C.Rotations[i]);
                    Batch[C.ISMIndex[i]].SetScale3D(FVector(Scale));
                }
            });

        ISM->BatchUpdateInstancesTransforms(0, Batch, true, true, true);
    }

    void RunDeath(UInstancedStaticMeshComponent* ISM)
    {
        if (!ISM) return;

        TArray<int32> ToRemoveISM;
        for (int32 ci = 0; ci < Chunks.Num(); ci++)
            for (int32 i = 0; i < Chunks[ci].Count; i++)
                if (Chunks[ci].HP[i] <= 0.f)
                    ToRemoveISM.Add(Chunks[ci].ISMIndex[i]);

        if (ToRemoveISM.Num() == 0) return;

        ToRemoveISM.Sort([](int32 A, int32 B) { return A > B; });
        for (int32 Idx : ToRemoveISM)
            if (Idx >= 0 && Idx < ISM->GetInstanceCount())
                ISM->RemoveInstance(Idx);

        for (int32 ci = Chunks.Num() - 1; ci >= 0; ci--)
            for (int32 i = Chunks[ci].Count - 1; i >= 0; i--)
            {
                if (Chunks[ci].HP[i] > 0.f) continue;

                FChunk& C = Chunks[ci];
                const int32 Last = C.Count - 1;

                if (i != Last)
                {
                    C.Positions[i] = C.Positions[Last];
                    C.Velocities[i] = C.Velocities[Last];
                    C.Rotations[i] = C.Rotations[Last];
                    C.FallVelocity[i] = C.FallVelocity[Last];
                    C.OnGround[i] = C.OnGround[Last];
                    C.GroundNormal[i] = C.GroundNormal[Last];
                    C.HP[i] = C.HP[Last];
                    C.AnimTime[i] = C.AnimTime[Last];
                    C.AnimStart[i] = C.AnimStart[Last];
                    C.AnimEnd[i] = C.AnimEnd[Last];
                    C.AnimRate[i] = C.AnimRate[Last];
                    C.ISMIndex[i] = C.ISMIndex[Last];
                    C.LODLevel[i] = C.LODLevel[Last];
                    C.DistSq[i] = C.DistSq[Last];
                }
                C.Count--;
            }
        for (int32 ci = Chunks.Num() - 1; ci >= 0; ci--)
            if (Chunks[ci].Count == 0)
                Chunks.RemoveAt(ci);

        int32 GlobalIdx = 0;
        for (FChunk& C : Chunks)
            for (int32 i = 0; i < C.Count; i++)
                C.ISMIndex[i] = GlobalIdx++;

    }
    

    void DamageEntitiesInRadius(const FVector& HitPos, float Radius, float Damage)
    {
        const float RadiusSq = Radius * Radius;

        for (FChunk& C : Chunks)
            for (int32 i = 0; i < C.Count; i++)
            {
                if (FVector::DistSquared(C.Positions[i], HitPos) < RadiusSq)
                    C.HP[i] -= Damage;
            }
    }

   
        
};