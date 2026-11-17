#include "Celda.h"

#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "PersonajeLadron.h"
#include "PersonajePolicia.h"

ACelda::ACelda()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Raiz")));

	ZonaInteraccion = CreateDefaultSubobject<UBoxComponent>(TEXT("ZonaInteraccion"));
	ZonaInteraccion->SetupAttachment(GetRootComponent());
	ZonaInteraccion->SetRelativeLocation(FVector(200.0f, 0.0f, 100.0f));
	ZonaInteraccion->SetBoxExtent(FVector(100.0f, 100.0f, 150.0f));
	ZonaInteraccion->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PuntoPrisionero = CreateDefaultSubobject<UArrowComponent>(TEXT("PuntoPrisionero"));
	PuntoPrisionero->SetupAttachment(GetRootComponent());
	PuntoPrisionero->ArrowColor = FColor::Red;
	PuntoSalida = CreateDefaultSubobject<UArrowComponent>(TEXT("PuntoSalida"));
	PuntoSalida->SetupAttachment(GetRootComponent());
	PuntoSalida->SetRelativeLocation(FVector(200.0f, 220.0f, 0.0f));
	PuntoSalida->ArrowColor = FColor::Green;
}

void ACelda::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACelda, Prisionero);
}

bool ACelda::EstaEnZona(const AActor* Interactor) const
{
	if (!IsValid(Interactor))
	{
		return false;
	}
	const FVector Local = ZonaInteraccion->GetComponentTransform().InverseTransformPosition(Interactor->GetActorLocation());
	return FBox(-ZonaInteraccion->GetUnscaledBoxExtent(), ZonaInteraccion->GetUnscaledBoxExtent()).IsInsideOrOn(Local);
}

ACelda* ACelda::BuscarEnZona(const AActor* Interactor, bool bSoloOcupadas)
{
	if (!IsValid(Interactor))
	{
		return nullptr;
	}

	ACelda* MasCercana = nullptr;
	double DistanciaMinima = TNumericLimits<double>::Max();
	for (TActorIterator<ACelda> It(Interactor->GetWorld()); It; ++It)
	{
		ACelda* Candidata = *It;
		const double Distancia = FVector::DistSquared(Interactor->GetActorLocation(), Candidata->ZonaInteraccion->GetComponentLocation());
		if (IsValid(Candidata) && Candidata->EstaEnZona(Interactor)
			&& (!bSoloOcupadas || IsValid(Candidata->Prisionero)) && Distancia < DistanciaMinima)
		{
			MasCercana = Candidata;
			DistanciaMinima = Distancia;
		}
	}
	return MasCercana;
}

bool ACelda::PuedeRescatar(const APersonajeLadron* Rescatador) const
{
	return IsValid(Rescatador) && Rescatador->PuedeActuarEnPartida() && Rescatador->GetController() != nullptr
		&& !Rescatador->bDerribado && !Rescatador->bSiendoCargado && !IsValid(Rescatador->CeldaActual)
		&& IsValid(Prisionero) && Prisionero != Rescatador && Prisionero->CeldaActual == this
		&& EstaEnZona(Rescatador);
}

bool ACelda::BuscarPosicionSegura(const UArrowComponent* Punto, const APersonajeLadron* Ladron,
	const AActor* ActorIgnorado, FVector& Posicion) const
{
	const UCapsuleComponent* Capsula = Ladron->GetCapsuleComponent();
	const float Semialtura = Capsula->GetScaledCapsuleHalfHeight();
	const FCollisionShape Forma = FCollisionShape::MakeCapsule(Capsula->GetScaledCapsuleRadius(), Semialtura);
	FCollisionQueryParams Parametros(SCENE_QUERY_STAT(ColocarPrisionero), false, Ladron);
	if (ActorIgnorado)
	{
		Parametros.AddIgnoredActor(ActorIgnorado);
	}
	const FVector Inicio = Punto->GetComponentLocation() + FVector(0.0f, 0.0f, Semialtura + 5.0f);
	if (GetWorld()->OverlapBlockingTestByChannel(Inicio, FQuat::Identity, ECC_Pawn, Forma, Parametros))
	{
		return false;
	}
	FHitResult Impacto;
	if (GetWorld()->SweepSingleByChannel(Impacto, Inicio, Inicio - FVector(0.0f, 0.0f, 200.0f),
		FQuat::Identity, ECC_Pawn, Forma, Parametros)
		&& !Impacto.bStartPenetrating && Ladron->GetCharacterMovement()->IsWalkable(Impacto))
	{
		Posicion = Impacto.Location + FVector(0.0f, 0.0f, 1.0f);
		return true;
	}
	return false;
}

bool ACelda::Encarcelar(APersonajePolicia* Policia, FString& Error)
{
	if (!HasAuthority() || !IsValid(Policia) || Policia->GetController() == nullptr || !EstaEnZona(Policia))
	{
		Error = TEXT("Debes estar en la zona de entrega de la celda.");
		return false;
	}
	if (IsValid(Prisionero))
	{
		Error = TEXT("Esta celda esta ocupada. Busca otra celda.");
		return false;
	}
	APersonajeLadron* Ladron = Policia->LadronCargado;
	if (!IsValid(Ladron) || !Ladron->bDerribado || !Ladron->bSiendoCargado
		|| IsValid(Ladron->CeldaActual) || Ladron->GetAttachParentActor() != Policia || Ladron->VidasRestantes <= 0)
	{
		Error = TEXT("No llevas un ladron que pueda ser encarcelado.");
		return false;
	}

	FVector Posicion;
	if (!BuscarPosicionSegura(PuntoPrisionero, Ladron, Policia, Posicion))
	{
		Error = TEXT("El punto interior de la celda no tiene espacio o suelo. Revisa PuntoPrisionero.");
		UE_LOG(LogTemp, Warning, TEXT("%s: %s"), *GetName(), *Error);
		return false;
	}

	Ladron->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	Ladron->SetActorLocationAndRotation(Posicion, PuntoPrisionero->GetComponentRotation(), false, nullptr, ETeleportType::TeleportPhysics);
	Ladron->CeldaActual = this;
	Ladron->EstablecerSiendoCargado(false);
	--Ladron->VidasRestantes;
	Prisionero = Ladron;
	Prisionero->OnDestroyed.AddDynamic(this, &ACelda::AlDestruirPrisionero);
	Ladron->ForceNetUpdate();
	ForceNetUpdate();
	return true;
}

bool ACelda::LiberarPrisionero(FString& Error)
{
	if (!HasAuthority() || !IsValid(Prisionero) || Prisionero->CeldaActual != this)
	{
		Error = TEXT("Esta celda ya no tiene un prisionero disponible.");
		return false;
	}
	FVector Posicion;
	// El rescatador puede estar sobre el marcador: se valida toda la capsula antes de liberar.
	if (!BuscarPosicionSegura(PuntoSalida, Prisionero, nullptr, Posicion))
	{
		Error = TEXT("La salida esta bloqueada o no tiene suelo. Deja libre el punto de salida e intentalo de nuevo.");
		return false;
	}

	APersonajeLadron* Liberado = Prisionero;
	Liberado->OnDestroyed.RemoveDynamic(this, &ACelda::AlDestruirPrisionero);
	Prisionero = nullptr;
	Liberado->SetActorLocationAndRotation(Posicion, PuntoSalida->GetComponentRotation(), false, nullptr, ETeleportType::TeleportPhysics);
	Liberado->CeldaActual = nullptr;
	Liberado->RecuperarSaludYMovimiento();
	ForceNetUpdate();
	return true;
}

void ACelda::AlDestruirPrisionero(AActor* ActorDestruido)
{
	if (Prisionero == ActorDestruido)
	{
		Prisionero = nullptr;
		ForceNetUpdate();
	}
}

void ACelda::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority() && IsValid(Prisionero) && Prisionero->CeldaActual == this)
	{
		Prisionero->OnDestroyed.RemoveDynamic(this, &ACelda::AlDestruirPrisionero);
		Prisionero->CeldaActual = nullptr;
		Prisionero->RecuperarSaludYMovimiento();
		Prisionero = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}
