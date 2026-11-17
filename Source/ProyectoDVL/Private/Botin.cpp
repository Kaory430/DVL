// Fill out your copyright notice in the Description page of Project Settings.


#include "Botin.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "EstadoJuegoDVL.h"
#include "ModoJuegoDVL.h"
#include "Net/UnrealNetwork.h"
#include "PersonajeLadron.h"
#include "PersonajePolicia.h"

// Sets default values
ABotin::ABotin()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(true);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	EsferaRecoleccion = CreateDefaultSubobject<USphereComponent>(TEXT("EsferaRecoleccion"));
	EsferaRecoleccion->SetupAttachment(Mesh);
	EsferaRecoleccion->InitSphereRadius(55.0f);
	EsferaRecoleccion->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	EsferaRecoleccion->SetCollisionResponseToAllChannels(ECR_Ignore);
	EsferaRecoleccion->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	EsferaRecoleccion->SetGenerateOverlapEvents(true);
	EsferaRecoleccion->OnComponentBeginOverlap.AddDynamic(this, &ABotin::OnRecoleccionOverlap);

}

void ABotin::OnRecoleccionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const AEstadoJuegoDVL* Partida = GetWorld()->GetGameState<AEstadoJuegoDVL>();
	if (HasAuthority() && (!Partida || Partida->PermiteAcciones()) && Estado == EEstadoBotin::Caido && Cast<APersonajePolicia>(OtherActor))
	{
		DevolverAlOrigen(true);
	}
}

// Called when the game starts or when spawned
void ABotin::BeginPlay()
{
	Super::BeginPlay();
	TransformacionOriginal = GetActorTransform();
	if (HasAuthority())
	{
		// Los BP existentes pueden conservar los valores de replicacion antiguos.
		SetReplicates(true);
		SetReplicateMovement(true);
	}
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetSimulatePhysics(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OnRep_Estado();
}

void ABotin::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABotin, NombreObjeto);
	DOREPLIFETIME(ABotin, AlcanceRobo);
	DOREPLIFETIME(ABotin, Estado);
}

void ABotin::OnRep_Estado()
{
	const bool bDisponible = Estado != EEstadoBotin::Transportado;
	Mesh->SetVisibility(bDisponible, true);
	EsferaRecoleccion->SetCollisionEnabled(bDisponible ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

bool ABotin::EstaAlAlcance(const APersonajeLadron* Ladron) const
{
	return IsValid(Ladron) && Estado != EEstadoBotin::Transportado
		&& FVector::DistSquared(Ladron->GetActorLocation(), EsferaRecoleccion->GetComponentLocation())
			<= FMath::Square(AlcanceRobo);
}

bool ABotin::Recoger(APersonajeLadron* Ladron)
{
	if (!HasAuthority() || !EstaAlAlcance(Ladron) || !Ladron->PuedeRobar())
	{
		return false;
	}
	Estado = EEstadoBotin::Transportado;
	++RevisionEstado;
	OnRep_Estado();
	ForceNetUpdate();
	return true;
}

void ABotin::Soltar(APersonajeLadron* Ladron, int32 Indice)
{
	if (!HasAuthority() || Estado != EEstadoBotin::Transportado || !Ladron)
	{
		return;
	}

	const float Angulo = FMath::DegreesToRadians(120.0f * Indice);
	const FVector Desplazamiento(FMath::Cos(Angulo) * 65.0f, FMath::Sin(Angulo) * 65.0f, 0.0f);
	FVector Inicio = Ladron->GetActorLocation() + Desplazamiento;
	FCollisionQueryParams Consulta(SCENE_QUERY_STAT(SoltarBotin), false, this);
	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		Consulta.AddIgnoredActor(*It);
	}
	FHitResult Obstaculo;
	if (GetWorld()->LineTraceSingleByChannel(Obstaculo, Ladron->GetActorLocation(), Inicio, ECC_Visibility, Consulta))
	{
		Inicio = Ladron->GetActorLocation();
	}
	FHitResult Suelo;
	if (!GetWorld()->LineTraceSingleByChannel(Suelo, Inicio, Inicio - FVector(0.0f, 0.0f, 600.0f), ECC_Visibility, Consulta))
	{
		UE_LOG(LogTemp, Warning, TEXT("No hay suelo para soltar %s; se devuelve a su origen."), *GetName());
		Ladron->Client_NotificarErrorInteraccion(TEXT("Un objeto no encontro suelo al caer y fue devuelto a su lugar."));
		DevolverAlOrigen();
		return;
	}

	const FBoxSphereBounds Limites = Mesh->CalcBounds(FTransform(TransformacionOriginal.GetRotation(), FVector::ZeroVector, GetActorScale3D()));
	FVector Posicion = Suelo.ImpactPoint;
	Posicion.Z += Limites.BoxExtent.Z - Limites.Origin.Z + 2.0f;
	SetActorLocationAndRotation(Posicion, TransformacionOriginal.Rotator(), false, nullptr, ETeleportType::TeleportPhysics);
	Estado = EEstadoBotin::Caido;
	++RevisionEstado;
	OnRep_Estado();
	ForceNetUpdate();
	ComprobarPoliciaCercano();
}

void ABotin::ComprobarPoliciaCercano()
{
	const AEstadoJuegoDVL* Partida = GetWorld()->GetGameState<AEstadoJuegoDVL>();
	if (Partida && !Partida->PermiteAcciones())
	{
		return;
	}
	EsferaRecoleccion->UpdateOverlaps();
	TArray<AActor*> Actores;
	EsferaRecoleccion->GetOverlappingActors(Actores, APersonajePolicia::StaticClass());
	if (!Actores.IsEmpty() && Estado == EEstadoBotin::Caido)
	{
		DevolverAlOrigen(true);
	}
}

void ABotin::DevolverAlOrigen(bool bRecuperadoPorPolicia)
{
	const bool bAvisar = bRecuperadoPorPolicia && Estado == EEstadoBotin::Caido;
	Estado = EEstadoBotin::EnOrigen;
	++RevisionEstado;
	SetActorTransform(TransformacionOriginal, false, nullptr, ETeleportType::TeleportPhysics);
	OnRep_Estado();
	ForceNetUpdate();
	if (bAvisar)
	{
		if (AModoJuegoDVL* Modo = GetWorld()->GetAuthGameMode<AModoJuegoDVL>())
		{
			Modo->AvisarDevolucion(this);
		}
	}
}
