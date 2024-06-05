// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/LobbyGameMode.h"
#include "LobbyGameMode.h"
#include "GameFramework/GameStateBase.h"
void ALobbyGameMode::PostLogin(APlayerController *NewPlayer)
{
    Super::PostLogin(NewPlayer);

    int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num(); //GameState is TObjPointer introduced in unreal 5. A special wrapper showing if pointer is in use
    if(NumberOfPlayers == 2)
    {
        UWorld* World = GetWorld();
        if(World)
        {
            bUseSeamlessTravel = true;
            World->ServerTravel(FString("/Game/Maps/BlasterMap?listen"));
        }
    }
    
}