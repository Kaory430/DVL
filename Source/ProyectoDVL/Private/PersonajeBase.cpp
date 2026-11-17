// Fill out your copyright notice in the Description page of Project Settings.


#include "PersonajeBase.h"

#include "HealthComponent.h"
#include "Engine/World.h"
#include "EstadoJuegoDVL.h"
#include "GameFramework/PlayerController.h"
#include "IndicadorInteraccion.h"
#include "PowerupComponent.h"

// Sets default values
APersonajeBase::APersonajeBase()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ComponenteSalud = CreateDefaultSubobject<UHealthComponent>(TEXT("ComponenteSalud"));
	ComponentePowerup = CreateDefaultSubobject<UPowerupComponent>(TEXT("ComponentePowerup"));

}

bool APersonajeBase::PuedeActuarEnPartida() const
{
	const AEstadoJuegoDVL* Estado = GetWorld()->GetGameState<AEstadoJuegoDVL>();
	return !Estado || Estado->PermiteAcciones();
}

// Called when the game starts or when spawned
void APersonajeBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APersonajeBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	APlayerController* Controlador = Cast<APlayerController>(GetController());
	if (Controlador && Controlador->IsLocalController())
	{
		if (!IndicadorInteraccion)
		{
			IndicadorInteraccion = CreateWidget<UIndicadorInteraccion>(Controlador);
			if (ensure(IndicadorInteraccion))
			{
				IndicadorInteraccion->AddToPlayerScreen();
			}
		}
	}
	else if (IndicadorInteraccion)
	{
		IndicadorInteraccion->RemoveFromParent();
		IndicadorInteraccion = nullptr;
	}
}

void APersonajeBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IndicadorInteraccion)
	{
		IndicadorInteraccion->RemoveFromParent();
		IndicadorInteraccion = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void APersonajeBase::UnPossessed()
{
	if (HasAuthority())
	{
		ComponentePowerup->CancelarEfecto();
	}
	Super::UnPossessed();
}

// Called to bind functionality to input
void APersonajeBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}
