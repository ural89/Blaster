// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterGameMode.h"
#include "Blaster/Private/Character/BlasterCharacter.h"
#include "Blaster/Private/PlayerController/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
void ABlasterGameMode::PlayerEliminated(ABlasterCharacter *ElimmedCharacter, ABlasterPlayerController *VictimController, ABlasterPlayerController *AttackerController)
{
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