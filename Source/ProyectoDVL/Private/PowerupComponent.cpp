#include "PowerupComponent.h"

#include "Celda.h"
#include "Components/SkeletalMeshComponent.h"
#include "ControladorJugadorDVL.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "EstadoJugadorDVL.h"
#include "EstadoJuegoDVL.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInterface.h"
#include "ModoJuegoDVL.h"
#include "Net/UnrealNetwork.h"
#include "PersonajeBase.h"
#include "PersonajeLadron.h"
#include "PersonajePolicia.h"
#include "UObject/ConstructorHelpers.h"

UPowerupComponent::UPowerupComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(
		TEXT("/Game/Powerups/M_Pulso.M_Pulso"));
	MaterialPulso = Material.Object;
}

void UPowerupComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!ensureMsgf(Cast<APersonajeBase>(GetOwner()), TEXT("PowerupComponent requiere un PersonajeBase.")))
	{
		SetComponentTickEnabled(false);
	}
}

void UPowerupComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UPowerupComponent, Carga, COND_OwnerOnly);
	DOREPLIFETIME(UPowerupComponent, Efecto);
	DOREPLIFETIME(UPowerupComponent, FinEfecto);
	DOREPLIFETIME_CONDITION(UPowerupComponent, ObjetivosPulso, COND_OwnerOnly);
}

bool UPowerupComponent::PuedeUsar() const
{
	const APersonajeBase* Personaje = Cast<APersonajeBase>(GetOwner());
	if (!Personaje || !Personaje->GetController() || !Personaje->PuedeActuarEnPartida())
	{
		return false;
	}
	if (const APersonajeLadron* Ladron = Cast<APersonajeLadron>(Personaje))
	{
		return !Ladron->bDerribado && !Ladron->bSiendoCargado && !IsValid(Ladron->CeldaActual);
	}
	return Cast<APersonajePolicia>(Personaje) != nullptr;
}

bool UPowerupComponent::PuedeRecoger(EPowerupDVL Tipo) const
{
	const bool bBandoCorrecto = (Tipo == EPowerupDVL::Pulso && Cast<APersonajePolicia>(GetOwner()))
		|| (Tipo == EPowerupDVL::Adrenalina && Cast<APersonajeLadron>(GetOwner()));
	return bBandoCorrecto && PuedeUsar() && Carga == EPowerupDVL::Ninguno && Efecto == EPowerupDVL::Ninguno;
}

bool UPowerupComponent::Recoger(EPowerupDVL Tipo)
{
	if (!GetOwner()->HasAuthority() || !PuedeRecoger(Tipo))
	{
		return false;
	}
	Carga = Tipo;
	GetOwner()->ForceNetUpdate();
	AvisarPropietario(Tipo == EPowerupDVL::Pulso
		? NSLOCTEXT("PowerupsDVL", "RecogerPulso", "Pulso de busqueda listo. Pulsa Q para activarlo.")
		: NSLOCTEXT("PowerupsDVL", "RecogerAdrenalina", "Adrenalina lista. Pulsa Q para activarla."));
	return true;
}

void UPowerupComponent::Activar()
{
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("La activacion de powerups debe ejecutarse en el servidor."));
		return;
	}
	if (!PuedeUsar() || Carga == EPowerupDVL::Ninguno || Efecto != EPowerupDVL::Ninguno)
	{
		AvisarPropietario(NSLOCTEXT("PowerupsDVL", "NoDisponible", "No tienes un powerup disponible o no puedes usarlo ahora."));
		return;
	}
	if (Carga == EPowerupDVL::Pulso && !MaterialPulso)
	{
		UE_LOG(LogTemp, Error, TEXT("Falta /Game/Powerups/M_Pulso. No se consume la carga."));
		AvisarPropietario(NSLOCTEXT("PowerupsDVL", "FaltaMaterial", "No se puede activar el pulso: falta su material visual."));
		return;
	}
	Efecto = Carga;
	Carga = EPowerupDVL::Ninguno;
	const AEstadoJuegoDVL* Estado = GetWorld()->GetGameState<AEstadoJuegoDVL>();
	const double Ahora = Estado ? Estado->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
	FinEfecto = Ahora + (Efecto == EPowerupDVL::Pulso ? 5.0 : 6.0);
	OnRep_Efecto();
	ComprobarEfecto();
	GetWorld()->GetTimerManager().SetTimer(TemporizadorEfecto, this, &UPowerupComponent::ComprobarEfecto, 0.1f, true);
	if (Efecto == EPowerupDVL::Pulso)
	{
		if (AModoJuegoDVL* Modo = GetWorld()->GetAuthGameMode<AModoJuegoDVL>())
		{
			Modo->AvisarPulso();
		}
		AvisarPropietario(NSLOCTEXT("PowerupsDVL", "ActivarPulso", "Pulso activo durante 5 segundos: busca siluetas a menos de 15 metros."));
	}
	else
	{
		AvisarPropietario(NSLOCTEXT("PowerupsDVL", "ActivarAdrenalina", "Adrenalina activa: velocidad +30% durante 6 segundos."));
	}
	GetOwner()->ForceNetUpdate();
}

float UPowerupComponent::SegundosRestantes() const
{
	const AEstadoJuegoDVL* Estado = GetWorld()->GetGameState<AEstadoJuegoDVL>();
	const double Ahora = Estado ? Estado->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
	return Efecto == EPowerupDVL::Ninguno ? 0.0f : FMath::Max(0.0f, static_cast<float>(FinEfecto - Ahora));
}

void UPowerupComponent::OnRep_Efecto()
{
	APersonajeBase* Personaje = Cast<APersonajeBase>(GetOwner());
	if (!Personaje)
	{
		return;
	}
	UCharacterMovementComponent* Movimiento = Personaje->GetCharacterMovement();
	if (Efecto == EPowerupDVL::Adrenalina && !bVelocidadAplicada)
	{
		VelocidadAnterior = Movimiento->MaxWalkSpeed;
		Movimiento->MaxWalkSpeed = VelocidadAnterior * 1.3f;
		bVelocidadAplicada = true;
	}
	else if (Efecto != EPowerupDVL::Adrenalina && bVelocidadAplicada)
	{
		Movimiento->MaxWalkSpeed = VelocidadAnterior;
		bVelocidadAplicada = false;
	}
}

void UPowerupComponent::ComprobarEfecto()
{
	if (!PuedeUsar() || SegundosRestantes() <= 0.0f)
	{
		CancelarEfecto();
		return;
	}
	if (Efecto == EPowerupDVL::Pulso)
	{
		TArray<TObjectPtr<APersonajeLadron>> NuevosObjetivos;
		for (TActorIterator<APersonajeLadron> It(GetWorld()); It; ++It)
		{
			if (PuedeDetectar(*It))
			{
				NuevosObjetivos.Add(*It);
			}
		}
		if (ObjetivosPulso != NuevosObjetivos)
		{
			ObjetivosPulso = MoveTemp(NuevosObjetivos);
			GetOwner()->ForceNetUpdate();
		}
	}
}

bool UPowerupComponent::PuedeDetectar(const APersonajeLadron* Ladron) const
{
	if (!Cast<APersonajePolicia>(GetOwner()) || !IsValid(Ladron) || IsValid(Ladron->CeldaActual))
	{
		return false;
	}
	// Los PlayerController de otros jugadores no existen en el cliente del policia.
	if (GetOwner()->HasAuthority() && !Ladron->GetController())
	{
		return false;
	}
	const AEstadoJugadorDVL* Estado = Ladron->GetPlayerState<AEstadoJugadorDVL>();
	return (!Estado || !Estado->bEliminado)
		&& FVector::DistSquared(GetOwner()->GetActorLocation(), Ladron->GetActorLocation()) <= FMath::Square(1500.0f);
}

void UPowerupComponent::CancelarEfecto()
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	GetWorld()->GetTimerManager().ClearTimer(TemporizadorEfecto);
	Efecto = EPowerupDVL::Ninguno;
	FinEfecto = 0.0;
	ObjetivosPulso.Reset();
	OnRep_Efecto();
	LimpiarSiluetas();
	GetOwner()->ForceNetUpdate();
}

void UPowerupComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const APersonajeBase* Personaje = Cast<APersonajeBase>(GetOwner());
	if (Personaje && Personaje->IsLocallyControlled() && Efecto == EPowerupDVL::Pulso && PuedeUsar() && SegundosRestantes() > 0.0f)
	{
		ActualizarSiluetas();
	}
	else if (!Siluetas.IsEmpty())
	{
		LimpiarSiluetas();
	}
}

void UPowerupComponent::ActualizarSiluetas()
{
	for (auto It = Siluetas.CreateIterator(); It; ++It)
	{
		if (!PuedeDetectar(It.Key()) || !ObjetivosPulso.Contains(It.Key()))
		{
			if (IsValid(It.Value()))
			{
				RestaurarAnimacion(Cast<USkeletalMeshComponent>(It.Value()->LeaderPoseComponent.Get()));
				It.Value()->DestroyComponent();
			}
			It.RemoveCurrent();
		}
	}
	for (APersonajeLadron* Objetivo : ObjetivosPulso)
	{
		if (!PuedeDetectar(Objetivo) || Siluetas.Contains(Objetivo))
		{
			continue;
		}
		USkeletalMeshComponent* Origen = Objetivo->GetMesh();
		if (!Origen || !Origen->GetSkeletalMeshAsset() || !MaterialPulso)
		{
			continue;
		}
		USkeletalMeshComponent* Silueta = NewObject<USkeletalMeshComponent>(GetOwner());
		AnimacionesPrevias.Add(Origen, Origen->VisibilityBasedAnimTickOption);
		Origen->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		Silueta->SetupAttachment(Origen);
		Silueta->SetSkeletalMeshAsset(Origen->GetSkeletalMeshAsset());
		Silueta->SetLeaderPoseComponent(Origen);
		Silueta->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Silueta->SetCastShadow(false);
		Silueta->SetOnlyOwnerSee(true);
		Silueta->SetReceivesDecals(false);
		Silueta->SetRenderCustomDepth(false);
		for (int32 Indice = 0; Indice < Silueta->GetNumMaterials(); ++Indice)
		{
			Silueta->SetMaterial(Indice, MaterialPulso);
		}
		Silueta->RegisterComponent();
		Siluetas.Add(Objetivo, Silueta);
	}
}

void UPowerupComponent::LimpiarSiluetas()
{
	for (const auto& Entrada : Siluetas)
	{
		if (IsValid(Entrada.Value))
		{
			Entrada.Value->DestroyComponent();
		}
	}
	Siluetas.Reset();
	for (const auto& Entrada : AnimacionesPrevias)
	{
		if (USkeletalMeshComponent* Origen = Entrada.Key.Get())
		{
			Origen->VisibilityBasedAnimTickOption = Entrada.Value;
		}
	}
	AnimacionesPrevias.Reset();
}

void UPowerupComponent::RestaurarAnimacion(USkeletalMeshComponent* Origen)
{
	EVisibilityBasedAnimTickOption Anterior;
	if (Origen && AnimacionesPrevias.RemoveAndCopyValue(Origen, Anterior))
	{
		Origen->VisibilityBasedAnimTickOption = Anterior;
	}
}

void UPowerupComponent::AvisarPropietario(const FText& Mensaje) const
{
	const APersonajeBase* Personaje = Cast<APersonajeBase>(GetOwner());
	if (AControladorJugadorDVL* Controlador = Personaje ? Cast<AControladorJugadorDVL>(Personaje->GetController()) : nullptr)
	{
		Controlador->Client_AvisoPartida(Mensaje);
	}
}

void UPowerupComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearTimer(TemporizadorEfecto);
	Efecto = EPowerupDVL::Ninguno;
	OnRep_Efecto();
	LimpiarSiluetas();
	Super::EndPlay(EndPlayReason);
}
