// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterGameMode.h"
#include "Blaster/Private/Character/BlasterCharacter.h"
#include "Blaster/Private/PlayerController/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "PlayerState/BlasterPlayerState.h"

namespace MatchState
{
    const FName Cooldown = FName("Cooldown");
}
ABlasterGameMode::ABlasterGameMode()
{
    bDelayedStart = true; // this spawns ghosts instead of pawn at start
}

void ABlasterGameMode::BeginPlay()
{
    Super::BeginPlay();
    LevelStartingTime = GetWorld()->GetTimeSeconds();
}
void ABlasterGameMode::OnMatchStateSet()
{
    Super::OnMatchStateSet();

    // tell all player controllers the gamestate
    for (FConstPlayerControllerIterator It =
             GetWorld()->GetPlayerControllerIterator();
         It; It++)
    {
        ABlasterPlayerController *BlasterPlayer = Cast<ABlasterPlayerController>(*It);
        if (BlasterPlayer)
        {
            BlasterPlayer->OnMatchStateSet(MatchState);
        }
    }
}
void ABlasterGameMode::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (MatchState == MatchState::WaitingToStart)
    {
        // since game mode runs in server only server holds time
        CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
        if (CountdownTime <= 0.f)
        {
            StartMatch();
        }
    }
    else if (MatchState == MatchState::InProgress)
    {
        CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
        if (CountdownTime)
            if (CountdownTime <= 0.f)
            {
                SetMatchState(MatchState::Cooldown);
            }
    }
    else if (MatchState == MatchState::Cooldown)
    {
        CountdownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
        if (CountdownTime <= 0.f)
        {
            RestartGame();
        }
    }
}

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter *ElimmedCharacter, ABlasterPlayerController *VictimController, ABlasterPlayerController *AttackerController)
{
    ABlasterPlayerState *AttackerPlayerState =
        AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
    ABlasterPlayerState *VictimPlayerState =
        VictimController ? Cast<ABlasterPlayerState>(VictimController->PlayerState) : nullptr;
    if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState)
    {
        AttackerPlayerState->AddToScore(1.f);
    }
    if (VictimPlayerState)
    {
        VictimPlayerState->AddToDefeats(1);
    }

    if (ElimmedCharacter)
    {
        ElimmedCharacter->Elim();
    }
}
void ABlasterGameMode::RequestRespawn(class ACharacter *ElimmedCharacter,
                                      AController *ElimmedController)
{
    if (ElimmedCharacter)
    {
        ElimmedCharacter->Reset(); // this detaches the controller so controller lives
        ElimmedCharacter->Destroy();
    }
    if (ElimmedController)
    {
        TArray<AActor *> StartPointActors;
        UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(),
                                              StartPointActors);
        int32 RandomIndex = FMath::RandRange(0, StartPointActors.Num() - 1);
        RestartPlayerAtPlayerStart(ElimmedController, StartPointActors[RandomIndex]);
    }
}
