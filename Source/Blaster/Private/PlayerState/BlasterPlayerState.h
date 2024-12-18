// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BlasterPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class ABlasterPlayerState : public APlayerState
{
	GENERATED_BODY()
public:
	virtual void OnRep_Score() override; //this is playerstate built in
	void AddToScore(float ScoreAmount);
private:
	class ABlasterCharacter* Character;
	class ABlasterPlayerController* Controller;
	
};
