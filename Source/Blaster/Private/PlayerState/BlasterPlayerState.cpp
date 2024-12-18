// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerState/BlasterPlayerState.h"
#include "BlasterPlayerState.h"
#include "Character/BlasterCharacter.h"
#include "PlayerController/BlasterPlayerController.h"
#include "Net/UnrealNetwork.h"

void ABlasterPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ABlasterPlayerState, Defeats);

}
void ABlasterPlayerState::AddToScore(float ScoreAmount) // this is for server
{
    SetScore(GetScore() + ScoreAmount);
    Character = Character == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : Character;
    if (Character)
    {
        Controller = Controller ==
                             nullptr
                         ? Cast<ABlasterPlayerController>(Character->Controller)
                         : Controller;

        if (Controller)
        {
            Controller->SetHUDScore(GetScore());
        }
    }
}

void ABlasterPlayerState::OnRep_Score() // reps only in clients not in server!!
{
    Super::OnRep_Score();
    Character = Character == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : Character;
    if (Character)
    {
        Controller = Controller ==
                             nullptr
                         ? Cast<ABlasterPlayerController>(Character->Controller)
                         : Controller;

        if (Controller)
        {
            Controller->SetHUDScore(GetScore());
        }
    }
}
void ABlasterPlayerState::AddToDefeats(int DefeatsAmount)
{
    Defeats += DefeatsAmount;
    Character = Character == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : Character;
    if (Character)
    {
        Controller = Controller ==
                             nullptr
                         ? Cast<ABlasterPlayerController>(Character->Controller)
                         : Controller;

        if (Controller)
        {
            Controller->SetHUDDefeats(Defeats);
        }
    }
}

void ABlasterPlayerState::OnRep_Defeats()
{
    Character = Character == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : Character;
    if (Character)
    {
        Controller = Controller ==
                             nullptr
                         ? Cast<ABlasterPlayerController>(Character->Controller)
                         : Controller;

        if (Controller)
        {
            Controller->SetHUDDefeats(Defeats);
        }
    }
}

