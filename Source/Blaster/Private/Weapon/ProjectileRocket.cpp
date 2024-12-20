// Fill out your copyright notice in the Description page of Project Settings.

#include "ProjectileRocket.h"
#include "Kismet/GameplayStatics.h"

AProjectileRocket::AProjectileRocket()
{
    RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RocketMesh"));
    RocketMesh->SetupAttachment(RootComponent);
    RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AProjectileRocket::OnHit(UPrimitiveComponent *HitComp,
                              AActor *OtherActor, UPrimitiveComponent *OtherComp,
                              FVector NormalImpulse, const FHitResult &Hit)
{
    APawn *FiringPawn = GetInstigator(); // this is set in weapon when we spawn projectile
    if (FiringPawn)
    {
        AController *FiringController = FiringPawn->GetController();
        if (FiringController)
        {
            UGameplayStatics::ApplyRadialDamageWithFalloff(
                this,
                Damage,
                Damage / 2.f,
                GetActorLocation(), // Origin
                200.f,
                500.f,
                1.f,
                UDamageType::StaticClass(),
                TArray<AActor *>(),
                this,
                FiringController // Instigator controller
            );
        }
    }

    Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}