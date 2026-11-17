// Fill out your copyright notice in the Description page of Project Settings.


#include "PersonajePolicia.h"

#include "Celda.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "PersonajeLadron.h"

namespace
{
	constexpr float DistanciaCarga = 200.0f;
}

APersonajePolicia::APersonajePolicia()
{
	bReplicates = true;

	ColisionPorra = CreateDefaultSubobject<UBoxComponent>(TEXT("ColisionPorra"));
	ColisionPorra->SetupAttachment(GetMesh(), TEXT("ManoDerechaSocket"));
	ColisionPorra->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ColisionPorra->SetCollisionResponseToAllChannels(ECR_Overlap);
	ColisionPorra->SetGenerateOverlapEvents(true);
	ColisionPorra->OnComponentBeginOverlap.AddDynamic(this, &APersonajePolicia::OnPorraOverlap);
}

void APersonajePolicia::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APersonajePolicia, LadronCargado);
}

void APersonajePolicia::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		SoltarLadron(true);
	}
	GetWorldTimerManager().ClearTimer(TemporizadorAtaque);
	Super::EndPlay(EndPlayReason);
}

APersonajeLadron* APersonajePolicia::BuscarLadronParaCargar() const
{
	if (!PuedeActuarEnPartida() || IsValid(LadronCargado) || GetController() == nullptr)
	{
		return nullptr;
	}

	APersonajeLadron* MasCercano = nullptr;
	float DistanciaCuadrada = FMath::Square(DistanciaCarga);
	for (TActorIterator<APersonajeLadron> It(GetWorld()); It; ++It)
	{
		APersonajeLadron* Candidato = *It;
		const float Distancia = FVector::DistSquared(GetActorLocation(), Candidato->GetActorLocation());
		if (IsValid(Candidato) && Candidato->bDerribado && !Candidato->bSiendoCargado
			&& !IsValid(Candidato->CeldaActual) && Distancia <= DistanciaCuadrada)
		{
			MasCercano = Candidato;
			DistanciaCuadrada = Distancia;
		}
	}
	return MasCercano;
}

void APersonajePolicia::CargarOSoltarLadron()
{
	Server_CargarOSoltarLadron();
}

void APersonajePolicia::Server_CargarOSoltarLadron_Implementation()
{
	if (!PuedeActuarEnPartida())
	{
		return;
	}
	if (IsValid(LadronCargado))
	{
		if (ACelda* Celda = ACelda::BuscarEnZona(this))
		{
			APersonajeLadron* Objetivo = LadronCargado;
			FString Error;
			if (Celda->Encarcelar(this, Error))
			{
				Objetivo->OnDestroyed.RemoveDynamic(this, &APersonajePolicia::AlDestruirLadronCargado);
				LadronCargado = nullptr;
				ForceNetUpdate();
			}
			else
			{
				Client_NotificarErrorCarga(Error);
			}
			return;
		}
		SoltarLadron();
		return;
	}

	APersonajeLadron* Objetivo = BuscarLadronParaCargar();
	if (!Objetivo)
	{
		Client_NotificarErrorCarga(TEXT("No hay un ladron derribado disponible a menos de 200 unidades."));
		return;
	}

	Objetivo->EstablecerSiendoCargado(true);
	// La capsula no depende de sockets ni de que el servidor actualice animaciones.
	if (!Objetivo->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale))
	{
		Objetivo->EstablecerSiendoCargado(false);
		UE_LOG(LogTemp, Error, TEXT("No se pudo adjuntar %s a %s."), *Objetivo->GetName(), *GetName());
		Client_NotificarErrorCarga(TEXT("No se pudo recoger al ladron."));
		return;
	}

	Objetivo->SetActorRelativeLocation(FVector(70.0f, 0.0f, 70.0f));
	LadronCargado = Objetivo;
	Objetivo->OnDestroyed.AddDynamic(this, &APersonajePolicia::AlDestruirLadronCargado);
	Objetivo->ForceNetUpdate();
	ForceNetUpdate();
}

bool APersonajePolicia::BuscarPosicionParaSoltar(FVector& Posicion) const
{
	if (!IsValid(LadronCargado))
	{
		return false;
	}

	const UCapsuleComponent* Capsula = LadronCargado->GetCapsuleComponent();
	const float Radio = Capsula->GetScaledCapsuleRadius();
	const float Semialtura = Capsula->GetScaledCapsuleHalfHeight();
	const FCollisionShape Forma = FCollisionShape::MakeCapsule(Radio, Semialtura);
	FCollisionQueryParams Parametros(SCENE_QUERY_STAT(SoltarLadron), false, this);
	Parametros.AddIgnoredActor(LadronCargado);
	const float Separacion = GetCapsuleComponent()->GetScaledCapsuleRadius() + Radio + 30.0f;
	const FVector Direcciones[] = { GetActorForwardVector(), -GetActorForwardVector(),
		GetActorRightVector(), -GetActorRightVector() };

	for (const FVector& Direccion : Direcciones)
	{
		const FVector Inicio = GetActorLocation() + Direccion * Separacion + FVector(0.0f, 0.0f, 10.0f);
		if (GetWorld()->OverlapBlockingTestByChannel(Inicio, FQuat::Identity, ECC_Pawn, Forma, Parametros))
		{
			continue;
		}

		FHitResult Impacto;
		if (GetWorld()->SweepSingleByChannel(Impacto, Inicio, Inicio - FVector(0.0f, 0.0f, 200.0f),
			FQuat::Identity, ECC_Pawn, Forma, Parametros)
			&& !Impacto.bStartPenetrating && LadronCargado->GetCharacterMovement()->IsWalkable(Impacto))
		{
			Posicion = Impacto.Location + FVector(0.0f, 0.0f, 1.0f);
			return true;
		}
	}
	return false;
}

void APersonajePolicia::SoltarLadron(bool bFinDeJuego)
{
	if (!IsValid(LadronCargado))
	{
		return;
	}

	FVector Posicion;
	if (!BuscarPosicionParaSoltar(Posicion))
	{
		if (!bFinDeJuego)
		{
			Client_NotificarErrorCarga(TEXT("No hay suelo o espacio libre para soltarlo. Alejate de la pared."));
			return;
		}
		// El portador desaparece; su posicion queda libre para el prisionero.
		Posicion = GetActorLocation();
	}

	APersonajeLadron* Objetivo = LadronCargado;
	Objetivo->OnDestroyed.RemoveDynamic(this, &APersonajePolicia::AlDestruirLadronCargado);
	Objetivo->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	Objetivo->SetActorLocation(Posicion, false, nullptr, ETeleportType::TeleportPhysics);
	Objetivo->EstablecerSiendoCargado(false);
	LadronCargado = nullptr;
	Objetivo->ForceNetUpdate();
	ForceNetUpdate();
}

void APersonajePolicia::AlDestruirLadronCargado(AActor* ActorDestruido)
{
	if (LadronCargado == ActorDestruido)
	{
		LadronCargado = nullptr;
		ForceNetUpdate();
	}
}

void APersonajePolicia::Client_NotificarErrorCarga_Implementation(const FString& Mensaje)
{
	UE_LOG(LogTemp, Warning, TEXT("%s"), *Mensaje);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Yellow, Mensaje);
	}
}

void APersonajePolicia::Atacar()
{
	Server_RealizarAtaque();
}

void APersonajePolicia::Server_RealizarAtaque_Implementation()
{
	if (!PuedeActuarEnPartida())
	{
		return;
	}
    ColisionPorra->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    
    // Forzamos a Unreal a revisar qué hay dentro de la caja en este instante
    ColisionPorra->UpdateOverlaps(); 

    GetWorldTimerManager().SetTimer(TemporizadorAtaque, this, &APersonajePolicia::DesactivarColisionPorra, 0.2f, false);
}

void APersonajePolicia::DesactivarColisionPorra()
{
	ColisionPorra->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APersonajePolicia::OnPorraOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (OtherActor != nullptr && OtherActor != this && HasAuthority() && PuedeActuarEnPartida())
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("¡GOLPE EN EL SERVIDOR DETECTADO!"));
        }

        // CANDADO: Apagamos la caja en el instante que toca al rival por primera vez
        ColisionPorra->SetCollisionEnabled(ECollisionEnabled::NoCollision);

        UGameplayStatics::ApplyDamage(OtherActor, 25.0f, GetController(), this, UDamageType::StaticClass());
    }
}