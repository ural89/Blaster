// Fill out your copyright notice in the Description page of Project Settings.
#include "BlasterPlayerController.h"
#include "HUD/BlasterHUD.h"
#include "HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Character/BlasterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "GameMode/BlasterGameMode.h"
#include "HUD/Announcement.h"
#include "Kismet/GameplayStatics.h"
#include "Blaster/BlasterComponents/CombatComponent.h"

void ABlasterPlayerController::BeginPlay()
{
    Super::BeginPlay();
    BlasterHUD = Cast<ABlasterHUD>(GetHUD());
    ServerCheckMatchState();
}

void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ABlasterPlayerController, MatchState);
}
void ABlasterPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateHUDTime();
    CheckTimeSync(DeltaTime);
    PollInit();
}

void ABlasterPlayerController::CheckTimeSync(float DeltaTime)
{
    TimeSyncRunningTime += DeltaTime;
    if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
    {
        ServerRequestServerTime(GetWorld()->GetTimeSeconds());
        TimeSyncRunningTime = 0;
    }
}
void ABlasterPlayerController::UpdateHUDTime()
{
    float TimeLeft = 0.f;
    if (MatchState == MatchState::WaitingToStart)
        TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
    else if (MatchState == MatchState::InProgress)
        TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
    else if (MatchState == MatchState::Cooldown)
        TimeLeft = WarmupTime + MatchTime + CooldownTime - GetServerTime() + LevelStartingTime;

    uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);

    if (HasAuthority())
    {
        BlasterGameMode =
            BlasterGameMode == nullptr ? Cast<ABlasterGameMode>(
                                             UGameplayStatics::GetGameMode(this))
                                       : BlasterGameMode;
        if (BlasterGameMode)
        {
            SecondsLeft = FMath::CeilToInt(BlasterGameMode->GetCountdownTime() + LevelStartingTime);
        }
    }
    if (CountdownInt != SecondsLeft)
    {
        if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
        {
            SetHUDAnnouncementCountdown(TimeLeft);
        }
        if (MatchState == MatchState::InProgress)
        {
            SetHUDMatchCountdown(TimeLeft);
        }
    }

    CountdownInt = SecondsLeft;
}
void ABlasterPlayerController::PollInit()
{
    if (CharacterOverlay == nullptr)
    {
        if (BlasterHUD && BlasterHUD->CharacterOverlay)
        {
            CharacterOverlay = BlasterHUD->CharacterOverlay;
            if (CharacterOverlay)
            {
                SetHUDHealth(HUDHealth, HUDMaxHealth);
                SetHUDScore(HUDScore);
                SetHUDDefeats(HUDDefeats);
            }
        }
    }
}
void ABlasterPlayerController::ServerRequestServerTime_Implementation(
    float TimeOfClientRequest)
{
    float ServerTimeOfReceived = GetWorld()->GetTimeSeconds(); // since this is server,
                                                               // this is server time
    ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceived);
}
void ABlasterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerRecievedClientRequest)
{
    float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
    float CurrentServerTime = TimeServerRecievedClientRequest + (0.5f * RoundTripTime);
    ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

float ABlasterPlayerController::GetServerTime()
{
    if (HasAuthority())
        return GetWorld()->GetTimeSeconds();
    else
        return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void ABlasterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
    bool bHUDValid = BlasterHUD && BlasterHUD->CharacterOverlay && BlasterHUD->CharacterOverlay->HealthBar && BlasterHUD->CharacterOverlay->HealthText;
    if (bHUDValid)
    {
        const float HealthPercent = Health / MaxHealth;
        BlasterHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
        FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
        BlasterHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
    }
    else
    {
        bInitializeCharacterOverlay = true;
        HUDHealth = Health;
        HUDMaxHealth = MaxHealth;
    }
}

void ABlasterPlayerController::SetHUDScore(float Score)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
    bool bHUDValid = BlasterHUD &&
                     BlasterHUD->CharacterOverlay &&
                     BlasterHUD->CharacterOverlay->ScoreAmount;

    if (bHUDValid)
    {
        FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
        BlasterHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
    }
    else
    {
        bInitializeCharacterOverlay = true;
        HUDScore = Score;
    }
}

void ABlasterPlayerController::SetHUDDefeats(int32 Defeats)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
    bool bHUDValid = BlasterHUD &&
                     BlasterHUD->CharacterOverlay &&
                     BlasterHUD->CharacterOverlay->DefeatsAmount;

    if (bHUDValid)
    {
        FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
        BlasterHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
    }
    else
    {
        bInitializeCharacterOverlay = true;
        HUDDefeats = Defeats;
    }
}

void ABlasterPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
    bool bHUDValid = BlasterHUD &&
                     BlasterHUD->CharacterOverlay &&
                     BlasterHUD->CharacterOverlay->WeaponAmmoAmount;

    if (bHUDValid)
    {
        FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
        BlasterHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
    }
}

void ABlasterPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
    bool bHUDValid = BlasterHUD &&
                     BlasterHUD->CharacterOverlay &&
                     BlasterHUD->CharacterOverlay->CarriedAmmoAmount;

    if (bHUDValid)
    {
        FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
        BlasterHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
    }
}
void ABlasterPlayerController::OnPossess(APawn *InPawn)
{
    Super::OnPossess(InPawn);
    if (ABlasterCharacter *BlasterCharacter = Cast<ABlasterCharacter>(InPawn))
    {
        SetHUDHealth(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
    }
}

void ABlasterPlayerController::SetHUDAnnouncementCountdown(float Countdown)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
    bool bHUDValid = BlasterHUD &&
                     BlasterHUD->Announcement &&
                     BlasterHUD->Announcement->WarmupTime;

    if (bHUDValid)
    {
        if (Countdown < 0.f) // hide timer if less then 0
        {
            BlasterHUD->Announcement->WarmupTime->SetText(FText());
            return;
        }
        int32 Minutes = FMath::FloorToInt(Countdown / 60.f);
        int32 Seconds = Countdown - Minutes * 60;
        // 02d means 2 digits (for example 01)
        FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
        BlasterHUD->Announcement->WarmupTime->SetText(
            FText::FromString(CountdownText));
    }
}

void ABlasterPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
    bool bHUDValid = BlasterHUD &&
                     BlasterHUD->CharacterOverlay &&
                     BlasterHUD->CharacterOverlay->MatchCountdownText;

    if (bHUDValid)
    {
        if (CountdownTime < 0.f) // hide timer if less then 0
        {
            BlasterHUD->CharacterOverlay->MatchCountdownText->SetText(FText());
            return;
        }
        int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
        int32 Seconds = CountdownTime - Minutes * 60;
        // 02d means 2 digits (for example 01)
        FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
        BlasterHUD->CharacterOverlay->MatchCountdownText->SetText(
            FText::FromString(CountdownText));
    }
}
void ABlasterPlayerController::ReceivedPlayer()
{
    Super::ReceivedPlayer();
    if (IsLocalController())
    {
        ServerRequestServerTime(GetWorld()->GetTimeSeconds());
    }
}

void ABlasterPlayerController::OnMatchStateSet(FName State)
{
    MatchState = State;
    if (MatchState == MatchState::InProgress)
    {
        HandleMatchHasStarted();
    }
    else if (MatchState == MatchState::Cooldown)
    {
        HandleCooldown();
    }
}

void ABlasterPlayerController::OnRep_MatchState()
{
    if (MatchState == MatchState::InProgress)
    {
        HandleMatchHasStarted();
    }
    else if (MatchState == MatchState::Cooldown)
    {
        HandleCooldown();
    }
}
void ABlasterPlayerController::HandleMatchHasStarted()
{

    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
    if (BlasterHUD)
    {
        BlasterHUD->AddCharacterOverlay();
        if (BlasterHUD->Announcement)
        {
            BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
        }
    }
}

void ABlasterPlayerController::HandleCooldown()
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
    if (BlasterHUD)
    {
        BlasterHUD->CharacterOverlay->RemoveFromParent();
        if (BlasterHUD->Announcement &&
            BlasterHUD->Announcement->AnnouncementText &&
            BlasterHUD->Announcement->InfoText)
        {
            BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
            FString AnnouncementText("New Match Start In: ");
            BlasterHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));
            BlasterHUD->Announcement->InfoText->SetText(FText());
        }
    }
    ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
    if (BlasterCharacter && BlasterCharacter->GetCombat())
    {
        BlasterCharacter->bDisableGameplay = true;
        BlasterCharacter->GetCombat()->FireButtonPressed(false);
    }
}

void ABlasterPlayerController::ClientJoinMidgame_Implementation(FName StateOfMatch,
                                                                float Warmup, float Match, float Cooldown,
                                                                float StartingTime)
{
    WarmupTime = Warmup;
    MatchTime = Match;
    CooldownTime = Cooldown;
    LevelStartingTime = StartingTime;
    MatchState = StateOfMatch;
    OnMatchStateSet(MatchState);
    if (BlasterHUD && MatchState == MatchState::WaitingToStart)
    {
        BlasterHUD->AddAnnouncement();
    }
}

void ABlasterPlayerController::ServerCheckMatchState_Implementation()
{
    ABlasterGameMode *GameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
    if (GameMode)
    {
        WarmupTime = GameMode->WarmupTime;
        MatchTime = GameMode->MatchTime;
        CooldownTime = GameMode->CooldownTime;
        LevelStartingTime = GameMode->LevelStartingTime;
        MatchState = GameMode->GetMatchState();
        ClientJoinMidgame(MatchState, WarmupTime, MatchTime, CooldownTime, LevelStartingTime);
        if (BlasterHUD && MatchState == MatchState::WaitingToStart)
        {
            BlasterHUD->AddAnnouncement();
        }
    }
}
