// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerState/BlasterPlayerState.h"
#include "BlasterPlayerState.h"
#include "Character/BlasterCharacter.h"
#include "PlayerController/BlasterPlayerController.h"

void ABlasterPlayerState::AddToScore(float ScoreAmount) // this is for server
{
    Score += ScoreAmount;
    Character = Character == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : Character;
    if (Character)
    {
        Controller = Controller ==
                             nullptr
                         ? Cast<ABlasterPlayerController>(Character->Controller)
                         : Controller;

        if (Controller)
        {
            Controller->SetHUDScore(Score);
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
            Controller->SetHUDScore(Score);
        }
    }
}
