#include "Character/BlasterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/WidgetComponent.h"
#include "BlasterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/Weapon.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "BlasterAnimInstance.h"
#include "Blaster/Blaster.h"
#include "PlayerController/BlasterPlayerController.h"
#include "Blaster/Private/GameMode/BlasterGameMode.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "PlayerState/BlasterPlayerState.h"
#include "Weapon/WeaponTypes.h"

ABlasterCharacter::ABlasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	SpawnCollisionHandlingMethod =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	CombatComp = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat"));
	CombatComp->SetIsReplicated(true); // components do not need to be registered to replicated. this is enough

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block); // this is trace response (visiblity)

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f; // this is the update freq if nothing changes frequently
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly); // registers replicated object
	DOREPLIFETIME(ABlasterCharacter, Health);
	DOREPLIFETIME(ABlasterCharacter, bDisableGameplay);
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (CombatComp)
	{
		CombatComp->Character = this;
	}
}

void ABlasterCharacter::Destroyed()
{
	Super::Destroyed();
	if (ElimBotComponent)
	{
		// Destroy component is not replicated!!
		ElimBotComponent->DestroyComponent();
	}
	ABlasterGameMode *BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	bool bMatchNotInProgress = BlasterGameMode && BlasterGameMode->GetMatchState() !=
													  MatchState::InProgress;
	if (CombatComp && CombatComp->EquippedWeapon && bMatchNotInProgress)
	{
		CombatComp->EquippedWeapon->Destroy();
	}
}

void ABlasterCharacter::OnRep_ReplicatedMovement() // this is overridden in actor. It is called on replication (not as frequent as tick)
{
	Super::OnRep_ReplicatedMovement();
	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0;
}
void ABlasterCharacter::ElimTimerFInished() // only server
{
	ABlasterGameMode *BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
	if (BlasterGameMode)
	{
		BlasterGameMode->RequestRespawn(this, Controller);
	}
}
void ABlasterCharacter::Elim() // since this is called from game mode, this is only called
							   // in server (game mode only lives in server)
{
	if (CombatComp && CombatComp->EquippedWeapon)
	{
		CombatComp->EquippedWeapon->Dropped();
	}
	MulticastElim();
	GetWorldTimerManager().SetTimer(
		ElimTimer,
		this,
		&ABlasterCharacter::ElimTimerFInished,
		ElimDelay);
}

void ABlasterCharacter::MulticastElim_Implementation()
{
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDWeaponAmmo(0);
	}
	bElimmed = true;
	PlayElimMontage();
	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance =
			UMaterialInstanceDynamic::Create(DissolveMaterialInstance,
											 this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	StartDissolve();
	bDisableGameplay = true;
	if (CombatComp)
	{
		CombatComp->FireButtonPressed(false);
	}
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately(); // also freezes look around
	bDisableGameplay = true;

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Spawn elimbot
	if (ElimBotEffect)
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X,
								  GetActorLocation().Y,
								  GetActorLocation().Z + 200.f);
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ElimBotEffect,
			ElimBotSpawnPoint,
			GetActorRotation());
	}
	bool bHideSniperScope = IsLocallyControlled() &&
							CombatComp &&
							CombatComp->bAiming &&
							CombatComp->EquippedWeapon &&
							CombatComp->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle;
	UE_LOG(LogTemp, Warning, TEXT("Show sniper scope %i"), bHideSniperScope);
	if (bHideSniperScope)
	{
		ShowSniperScopeWidget(false);
	}
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	UpdateHUDHealth();
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ABlasterCharacter::RecieveDamage);
	}
}

void ABlasterCharacter::UpdateHUDHealth()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	RotateInPlace(DeltaTime);
	HideCameraIfCharacterClose();
	PollInit();
}
void ABlasterCharacter::RotateInPlace(float DeltaTime)
{
	if (bDisableGameplay)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}
	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy &&
		IsLocallyControlled()) // local role gives None,SimulatedProxy, AutonomousProxy, Authority, Max. This is hierarchyle.
							   // Bigger than simulated means actively local controlled or server
							   // This is like HasInputAuthority() like in photon?
	{
		UpdateAimOffset(DeltaTime);
	}
	else
	{
		TimeSinceLastMovementReplication += DeltaTime; // since OnRep_ReplicatedMovement only called when moving, we call manually when we are also not moving
		if (TimeSinceLastMovementReplication > 0.25f)
		{
			OnRep_ReplicatedMovement();
		}
		CalculateAO_Pitch();
	}
}
void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ABlasterCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ABlasterCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &ABlasterCharacter::LookUp);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &ABlasterCharacter::Turn);
	PlayerInputComponent->BindAction(TEXT("Jump"), EInputEvent::IE_Pressed, this, &ABlasterCharacter::Jump);
	PlayerInputComponent->BindAction(TEXT("Equip"), EInputEvent::IE_Pressed, this, &ABlasterCharacter::EquipButtonPressed);
	PlayerInputComponent->BindAction(TEXT("Crouch"), EInputEvent::IE_Pressed, this, &ABlasterCharacter::CrouchButtonPressed);
	PlayerInputComponent->BindAction(TEXT("Aim"), EInputEvent::IE_Pressed, this, &ABlasterCharacter::AimButtonPressed);
	PlayerInputComponent->BindAction(TEXT("Aim"), EInputEvent::IE_Released, this, &ABlasterCharacter::AimButtonReleased);
	PlayerInputComponent->BindAction(TEXT("Fire"), EInputEvent::IE_Pressed, this, &ABlasterCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction(TEXT("Fire"), EInputEvent::IE_Released, this, &ABlasterCharacter::FireButtonReleased);
	PlayerInputComponent->BindAction(TEXT("Reload"), EInputEvent::IE_Pressed, this, &ABlasterCharacter::ReloadButtonPressed);
}

void ABlasterCharacter::MoveForward(float Value)
{
	if (bDisableGameplay)
		return;
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0, Controller->GetControlRotation().Yaw, 0);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X)); // X is forward in Unreal
		AddMovementInput(Direction, Value);
	}
}

void ABlasterCharacter::MoveRight(float Value)
{
	if (bDisableGameplay)
		return;
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0, Controller->GetControlRotation().Yaw, 0);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, Value);
	}
}

void ABlasterCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void ABlasterCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}
void ABlasterCharacter::EquipButtonPressed()
{
	if (bDisableGameplay)
		return;
	if (CombatComp)
	{
		if (HasAuthority())
		{
			UE_LOG(LogTemp, Warning, TEXT("Has Auth equip"));
			CombatComp->EquipWeapon(OverlappingWeapon);
		}
		else // this is called from client
		{
			UE_LOG(LogTemp, Warning, TEXT("NoAuth equip"));
			ServerEquipButtonPressed(); // RPC call
		}
	}
}
void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	if (CombatComp) // we dont need to check Authority since this is RPC to server from client
	{
		CombatComp->EquipWeapon(OverlappingWeapon);
	}
}

void ABlasterCharacter::CrouchButtonPressed()
{
	if (bDisableGameplay)
		return;
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch(); // this uses bIsCrouched in unreal. And this bool is replicated. and has OnRep_IsCrouched() callback in character class.
				  // REMEMBER TO ENABLE CAN CROUCH IN CHAR BLUEPRINT
	}
}

void ABlasterCharacter::ReloadButtonPressed()
{
	if (bDisableGameplay)
		return;
	if (CombatComp)
	{
		CombatComp->Reload();
	}
}

void ABlasterCharacter::AimButtonPressed()
{
	if (bDisableGameplay)
		return;
	if (CombatComp)
	{
		CombatComp->SetAiming(true);
	}
}

void ABlasterCharacter::AimButtonReleased()
{
	if (bDisableGameplay)
		return;
	if (CombatComp)
	{
		CombatComp->SetAiming(false);
	}
}

void ABlasterCharacter::UpdateTurnInPlace(float DeltaTime)
{
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}
	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 10.f);
		AO_Yaw = InterpAO_Yaw;
		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			LastRunningAimRotation = FRotator(0, GetBaseAimRotation().Yaw, 0);
		}
	}
}
void ABlasterCharacter::UpdateAimOffset(float DeltaTime)
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0;
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	if (Velocity == FVector::ZeroVector && !bIsInAir) // standing still
	{
		bRotateRootBone = true;
		FRotator CurrentAimRotation = FRotator(0, GetBaseAimRotation().Yaw, 0);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, LastRunningAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}
		bUseControllerRotationYaw = true;
		UpdateTurnInPlace(DeltaTime);
	}
	else // if moving
	{
		bRotateRootBone = false;
		LastRunningAimRotation = FRotator(0, GetBaseAimRotation().Yaw, 0);
		AO_Yaw = 0;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	CalculateAO_Pitch();
}

float ABlasterCharacter::CalculateSpeed()
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	return Velocity.Size();
}
void ABlasterCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}
void ABlasterCharacter::SimProxiesTurn()
{
	if (CombatComp == nullptr)
		return;
	if (CombatComp->EquippedWeapon == nullptr)
		return;
	bRotateRootBone = false;
	float Speed = CalculateSpeed();
	if (Speed > 0)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}
	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	if (FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if (ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (ProxyYaw < -TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		return;
	}
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

void ABlasterCharacter::OnRep_Health()
{
	UpdateHUDHealth();
	PlayHitReactMontage();
}
void ABlasterCharacter::Jump()
{
	if (bDisableGameplay)
		return;
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Super::Jump();
	}
}

void ABlasterCharacter::RecieveDamage(AActor *DamagedActor, float Damage, const UDamageType *DamageType, AController *InstigatorController, AActor *DamageCauser)
{
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	UpdateHUDHealth();
	PlayHitReactMontage();
	if (Health <= 0.f)
	{
		ABlasterGameMode *BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
		if (BlasterGameMode)
		{
			BlasterPlayerController =
				BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
			ABlasterPlayerController *AttackerController =
				Cast<ABlasterPlayerController>(InstigatorController);
			BlasterGameMode->PlayerEliminated(this, BlasterPlayerController, AttackerController);
		}
	}
}

void ABlasterCharacter::PollInit()
{
	if (BlasterPlayerState == nullptr)
	{
		BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
		if (BlasterPlayerState)
		{
			BlasterPlayerState->AddToScore(0.f);
			BlasterPlayerState->AddToDefeats(0);
		}
	}
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon *LastWeapon) // Here lastWeapon will be the last value BEFORE change with replication
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

void ABlasterCharacter::HideCameraIfCharacterClose()
{
	if (!IsLocallyControlled())
		return;
	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		GetMesh()->SetVisibility(false);
		if (CombatComp && CombatComp->EquippedWeapon && CombatComp->EquippedWeapon->GetWeaponMesh())
		{
			CombatComp->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if (CombatComp && CombatComp->EquippedWeapon && CombatComp->EquippedWeapon->GetWeaponMesh())
		{
			CombatComp->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}

void ABlasterCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(
			TEXT("Dissolve"),
			DissolveValue);
	}
}

void ABlasterCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &ABlasterCharacter::UpdateDissolveMaterial);
	if (DissolveCurve && DissolveTimeline)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
	}
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon *Weapon)
{
	if (OverlappingWeapon) // set false before this is set to nullptr. This line is also server only
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}

	OverlappingWeapon = Weapon;
	if (IsLocallyControlled()) // is local player? //this line is for server. Since OnRep_OverlappingWeapon methods will be called for clients.
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

bool ABlasterCharacter::IsWeaponEquipped()
{

	return (CombatComp && CombatComp->EquippedWeapon);
}

bool ABlasterCharacter::IsAiming()
{
	return (CombatComp && CombatComp->bAiming);
}

AWeapon *ABlasterCharacter::GetEquippedWeapon()
{
	if (CombatComp == nullptr)
		return nullptr;
	return CombatComp->EquippedWeapon;
}

FVector ABlasterCharacter::GetHitTarget() const
{
	if (CombatComp == nullptr)
		return FVector();
	return CombatComp->HitTarget;
}

ECombatState ABlasterCharacter::GetCombatState() const
{
	if (CombatComp == nullptr)
		return ECombatState::ECS_MAX;
	return CombatComp->CombatState;
}

void ABlasterCharacter::FireButtonPressed()
{
	if (bDisableGameplay)
		return;
	if (CombatComp)
	{
		CombatComp->FireButtonPressed(true);
	}
}

void ABlasterCharacter::FireButtonReleased()
{
	if (bDisableGameplay)
		return;
	if (CombatComp)
	{
		CombatComp->FireButtonPressed(false);
	}
}

void ABlasterCharacter::PlayFireMontage(bool bAiming)
{
	if (CombatComp == nullptr || CombatComp->EquippedWeapon == nullptr)
		return;

	UAnimInstance *AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);

		FName SectionName;
		SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayReloadMontage()
{

	if (CombatComp == nullptr || CombatComp->EquippedWeapon == nullptr)
		return;

	UAnimInstance *AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);

		FName SectionName;
		switch (CombatComp->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRiffle:
			SectionName = FName("Riffle");
			break;
		case EWeaponType::EWT_RocketLauncher:
			SectionName = FName("RocketLauncher");
			break;
		case EWeaponType::EWT_Pistol:
			SectionName = FName("Pistol");
			break;
		case EWeaponType::EWT_SubmachineGun:
			SectionName = FName("Pistol");
			break;
		case EWeaponType::EWT_Shotgun:
			SectionName = FName("Shotgun");
			break;
		case EWeaponType::EWT_SniperRifle:
			SectionName = FName("SniperRifle");
			break;
		case EWeaponType::EWT_GrenadeLauncher:
			SectionName = FName("GrenadeLauncher");
			break;
		}
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayElimMontage()
{
	UAnimInstance *AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ElimMontage)
	{
		AnimInstance->Montage_Play(ElimMontage);
	}
}

void ABlasterCharacter::PlayHitReactMontage()
{
	if (CombatComp == nullptr || CombatComp->EquippedWeapon == nullptr)
		return;

	UAnimInstance *AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);

		FName SectionName("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}
