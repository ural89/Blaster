// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()
public:
	class UTexture2D *CrosshairsCenter;
	class UTexture2D *CrosshairsLeft;
	class UTexture2D *CrosshairsRight;
	class UTexture2D *CrosshairsTop;
	class UTexture2D *CrosshairsBottom;
	float CrosshairSpread;
};
/**
 *
 */
UCLASS()
class ABlasterHUD : public AHUD
{
	GENERATED_BODY()
public:
	void DrawHUD() override;

private:
	FHUDPackage HUDPackage;
	
	void DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread);

	UPROPERTY(EditAnywhere)
	float CrosshairSpreadMax = 16.f;
public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage &Package) { HUDPackage = Package; }
};
