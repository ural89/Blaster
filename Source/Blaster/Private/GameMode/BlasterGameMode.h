// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"

/**
 *
 */
UCLASS()
class ABlasterGameMode : public AGameMode // Gamemode has MatchStates on top of gamemodebase
{
	GENERATED_BODY()
public:
	ABlasterGameMode();
	virtual void Tick(float DeltaTime) override;
	virtual void PlayerEliminated(class ABlasterCharacter *ElimmedCharacter,
								  class ABlasterPlayerController *VictimController,
								  ABlasterPlayerController *AttackerController);

	virtual void RequestRespawn(class ACharacter *ElimmedCharacter,
								AController *ElimmedController);

	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;

	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 120.f;

	float LevelStartingTime = 0.f;

protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;

private:
	float CountdownTime = 0;
	
};
