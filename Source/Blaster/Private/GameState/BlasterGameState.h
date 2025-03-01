// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "BlasterGameState.generated.h"

/**
 *
 */
UCLASS()
class ABlasterGameState : public AGameState
{
	GENERATED_BODY()
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;
	void UpdateTopScore(class ABlasterPlayerState *ScoringPlayer);

	UPROPERTY(Replicated)
	TArray<ABlasterPlayerState *> TopScoringPlayers; // there could be a tie in lead
private:
	float TopScore = 0.f;
};
