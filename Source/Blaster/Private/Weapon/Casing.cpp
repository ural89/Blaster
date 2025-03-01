// Fill out your copyright notice in the Description page of Project Settings.


#include "Casing.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
ACasing::ACasing()
{
	PrimaryActorTick.bCanEverTick = false;

	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
	SetRootComponent(CasingMesh);

	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	CasingMesh->SetSimulatePhysics(true);
	CasingMesh->SetEnableGravity(true);
	CasingMesh->SetNotifyRigidBodyCollision(true);
	ShellEjectionImpulsePower = 10.f;


}

void ACasing::BeginPlay()
{
	Super::BeginPlay();
	CasingMesh->AddImpulse(GetActorForwardVector() * ShellEjectionImpulsePower);
	CasingMesh->OnComponentHit.AddDynamic(this, &ACasing::OnHit); //CasingMesh->SetNotifyRigidBodyCollision(true); neeede to run!!
	
}
void ACasing::LateDestroy()
{
	Destroy();
}
void ACasing::OnHit(UPrimitiveComponent *HitComp, AActor *OtherActor, UPrimitiveComponent *OtherComp, FVector NormalImpulse, const FHitResult &Hit)
{
	FTimerHandle DestroyTimerHandle; //you can simply use SetLifeSpawn(3.f) in begin play
	GetWorld()->GetTimerManager().SetTimer(DestroyTimerHandle, this, &ACasing::LateDestroy, 1.f);
	if (ShellSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ShellSound, GetActorLocation());
	}
}
