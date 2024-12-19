// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

/**
 *
 */
UCLASS()
class ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDCarriedAmmo(int32 Ammo);
	void SetHUDMatchCountdown(float CountDownTime);
	virtual void OnPossess(APawn *InPawn) override;
	virtual void Tick(float DeltaTime) override;

	virtual float GetServerTime();
	virtual void ReceivedPlayer() override; //Sync with server clock as soon as possible
	
protected:
	void BeginPlay() override;
	void UpdateHUDTime();

	// Sync time client and server

	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest,
								float TimeServerRecievedClientRequest);

	float ClientServerDelta = 0.f;
	
	UPROPERTY(EditAnywhere, Category = Time)
	float TimeSyncFrequency = 5.f;

	float TimeSyncRunningTime = 0.f;

	void CheckTimeSync(float DeltaTime);

private:
	UPROPERTY()
	class ABlasterHUD *BlasterHUD;

	float MatchTime = 120.f;
	uint32 CountdownInt = 0;
};
