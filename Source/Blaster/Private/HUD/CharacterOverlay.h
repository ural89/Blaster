// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterOverlay.generated.h"

/**
 * 
 */
UCLASS()
class UCharacterOverlay : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta=(BindWidget))//this is required to bind widget
	class UProgressBar* HealthBar;

	UPROPERTY(meta=(BindWidget))//this is required to bind widget
	class UTextBlock* HealthText;

	
};
