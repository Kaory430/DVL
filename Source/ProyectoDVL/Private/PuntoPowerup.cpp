#include "PuntoPowerup.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EstadoJuegoDVL.h"
#include "Net/UnrealNetwork.h"
#include "PersonajeBase.h"
#include "UObject/ConstructorHelpers.h"

APuntoPowerup::APuntoPowerup()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Raiz")));
	ZonaRecogida = CreateDefaultSubobject<USphereComponent>(TEXT("ZonaRecogida"));
	ZonaRecogida->SetupAttachment(GetRootComponent());
	ZonaRecogida->SetRelativeLocation(FVector(0.0f, 0.0f, 70.0f));
	ZonaRecogida->InitSphereRadius(90.0f);
	ZonaRecogida->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ZonaRecogida->SetCollisionResponseToAllChannels(ECR_Ignore);
	ZonaRecogida->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ZonaRecogida->SetGenerateOverlapEvents(true);
	ZonaRecogida->OnComponentBeginOverlap.AddDynamic(this, &APuntoPowerup::AlEntrar);
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(ZonaRecogida);
	Mesh->SetRelativeScale3D(FVector(0.55f));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Esfera(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	Mesh->SetStaticMesh(Esfera.Object);
	Texto = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Texto"));
	Texto->SetupAttachment(GetRootComponent());
	Texto->SetRelativeLocation(FVector(0.0f, 0.0f, 135.0f));
	Texto->SetHorizontalAlignment(EHTA_Center);
	Texto->SetWorldSize(18.0f);
	Luz = CreateDefaultSubobject<UPointLightComponent>(TEXT("Luz"));
	Luz->SetupAttachment(ZonaRecogida);
	Luz->SetIntensity(600.0f);
	Luz->SetAttenuationRadius(200.0f);
	Luz->SetCastShadows(false);
}

void APuntoPowerup::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	OnRep_Disponibilidad();
}

void APuntoPowerup::BeginPlay()
{
	Super::BeginPlay();
	if (!ensureMsgf(Tipo == EPowerupDVL::Pulso || Tipo == EPowerupDVL::Adrenalina, TEXT("Configura el tipo de %s."), *GetName()))
	{
		bDisponible = false;
	}
	OnRep_Disponibilidad();
}

void APuntoPowerup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APuntoPowerup, Tipo);
	DOREPLIFETIME(APuntoPowerup, bDisponible);
	DOREPLIFETIME(APuntoPowerup, ProximaReaparicion);
}

void APuntoPowerup::OnRep_Disponibilidad()
{
	const FColor Color = Tipo == EPowerupDVL::Pulso ? FColor(40, 150, 255) : FColor(255, 170, 35);
	Texto->SetText(Tipo == EPowerupDVL::Pulso
		? NSLOCTEXT("PowerupsDVL", "PuntoPulso", "PULSO\nSOLO POLICIA")
		: NSLOCTEXT("PowerupsDVL", "PuntoAdrenalina", "ADRENALINA\nSOLO LADRONES"));
	Texto->SetTextRenderColor(Color);
	Luz->SetLightColor(Color);
	Mesh->SetVisibility(bDisponible, true);
	Texto->SetVisibility(bDisponible);
	Luz->SetVisibility(bDisponible);
}

void APuntoPowerup::AlEntrar(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (HasAuthority())
	{
		IntentarRecoger(Cast<APersonajeBase>(OtherActor));
	}
}

bool APuntoPowerup::IntentarRecoger(APersonajeBase* Personaje)
{
	if (!HasAuthority() || !bDisponible || !IsValid(Personaje) || !ZonaRecogida->IsOverlappingActor(Personaje)
		|| !Personaje->ComponentePowerup->PuedeRecoger(Tipo))
	{
		return false;
	}
	// Se reserva antes de entregar para evitar dos recogidas por overlaps simultaneos.
	bDisponible = false;
	if (!Personaje->ComponentePowerup->Recoger(Tipo))
	{
		bDisponible = true;
		UE_LOG(LogTemp, Warning, TEXT("%s no pudo entregar el powerup a %s."), *GetName(), *Personaje->GetName());
		return false;
	}
	const AEstadoJuegoDVL* Estado = GetWorld()->GetGameState<AEstadoJuegoDVL>();
	ProximaReaparicion = (Estado ? Estado->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds()) + 90.0;
	OnRep_Disponibilidad();
	ForceNetUpdate();
	GetWorldTimerManager().SetTimer(TemporizadorReaparicion, this, &APuntoPowerup::Reaparecer, 90.0f, false);
	return true;
}

void APuntoPowerup::Reaparecer()
{
	bDisponible = true;
	ProximaReaparicion = 0.0;
	OnRep_Disponibilidad();
	ForceNetUpdate();
	ZonaRecogida->UpdateOverlaps();
	TArray<AActor*> Dentro;
	ZonaRecogida->GetOverlappingActors(Dentro, APersonajeBase::StaticClass());
	for (AActor* Actor : Dentro)
	{
		if (IntentarRecoger(Cast<APersonajeBase>(Actor)))
		{
			break;
		}
	}
}

void APuntoPowerup::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(TemporizadorReaparicion);
	Super::EndPlay(EndPlayReason);
}
