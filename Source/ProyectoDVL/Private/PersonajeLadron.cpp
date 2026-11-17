// Fill out your copyright notice in the Description page of Project Settings.


#include "PersonajeLadron.h"

#include "Celda.h"
#include "Botin.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "EstadoJugadorDVL.h"
#include "EstadoJuegoDVL.h"
#include "ModoJuegoDVL.h"
#include "PersonajePolicia.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "HealthComponent.h"
#include "PowerupComponent.h"
#include "Net/UnrealNetwork.h"

namespace
{
	constexpr float DistanciaReanimacion = 200.0f;
	constexpr double DuracionReanimacion = 10.0;
	constexpr double DuracionRobo = 10.0;
}

APersonajeLadron::APersonajeLadron()
{
	bReplicates = true;
}

void APersonajeLadron::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		ComponenteSalud->OnSaludAgotada.AddDynamic(this, &APersonajeLadron::AlAgotarseSalud);
	}
}

void APersonajeLadron::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(TemporizadorReanimacion);
	GetWorldTimerManager().ClearTimer(TemporizadorRobo);
	if (HasAuthority() && EndPlayReason == EEndPlayReason::Destroyed)
	{
		SoltarInventario();
	}
	Super::EndPlay(EndPlayReason);
}

void APersonajeLadron::UnPossessed()
{
	if (HasAuthority())
	{
		CancelarReanimacionServidor();
		SoltarInventario();
	}
	Super::UnPossessed();
}

void APersonajeLadron::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APersonajeLadron, VidasRestantes);
	DOREPLIFETIME(APersonajeLadron, bDerribado);
	DOREPLIFETIME(APersonajeLadron, bSiendoCargado);
	DOREPLIFETIME(APersonajeLadron, bReanimando);
	DOREPLIFETIME(APersonajeLadron, CeldaActual);
	DOREPLIFETIME_CONDITION(APersonajeLadron, bRescatando, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(APersonajeLadron, ProgresoReanimacion, COND_OwnerOnly);
	DOREPLIFETIME(APersonajeLadron, Inventario);
	DOREPLIFETIME(APersonajeLadron, BotinRecolectado);
	DOREPLIFETIME(APersonajeLadron, bRobando);
	DOREPLIFETIME(APersonajeLadron, ObjetivoRobo);
	DOREPLIFETIME_CONDITION(APersonajeLadron, ProgresoRobo, COND_OwnerOnly);
}

void APersonajeLadron::OnRep_SiendoCargado()
{
	SetActorEnableCollision(!bSiendoCargado);
	OnRep_Derribado();
}

void APersonajeLadron::OnRep_CeldaActual()
{
	OnRep_Derribado();
}

void APersonajeLadron::FaceRotation(FRotator NewControlRotation, float DeltaTime)
{
	if (!bDerribado && !bSiendoCargado && !IsValid(CeldaActual))
	{
		Super::FaceRotation(NewControlRotation, DeltaTime);
	}
}

void APersonajeLadron::AlAgotarseSalud()
{
	if (bDerribado || IsValid(CeldaActual))
	{
		return;
	}

	CancelarReanimacionServidor();
	ComponentePowerup->CancelarEfecto();
	if (AModoJuegoDVL* Modo = GetWorld()->GetAuthGameMode<AModoJuegoDVL>())
	{
		Modo->AvisarDerribo(this, Cast<APersonajePolicia>(ComponenteSalud->UltimoCausanteDano.Get()) != nullptr);
	}
	SoltarInventario();
	if (VidasRestantes <= 0)
	{
		Morir();
		return;
	}

	bDerribado = true;
	OnRep_Derribado();
	ForceNetUpdate();
}

void APersonajeLadron::OnRep_Derribado()
{
	if (bDerribado || bSiendoCargado || IsValid(CeldaActual))
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}
	else
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}

void APersonajeLadron::IniciarReanimacion()
{
	Server_IniciarReanimacion();
}

void APersonajeLadron::IniciarInteraccion()
{
	Server_IniciarReanimacion();
}

void APersonajeLadron::CancelarInteraccion()
{
	Server_CancelarReanimacion();
}

void APersonajeLadron::CancelarReanimacion()
{
	Server_CancelarReanimacion();
}

bool APersonajeLadron::PuedeReanimar(const APersonajeLadron* Objetivo) const
{
	return IsValid(Objetivo) && Objetivo != this && GetController() != nullptr
		&& PuedeActuarEnPartida() && !bDerribado && !bSiendoCargado && Objetivo->bDerribado
		&& !IsValid(CeldaActual) && !IsValid(Objetivo->CeldaActual) && !Objetivo->bSiendoCargado
		&& FVector::DistSquared(GetActorLocation(), Objetivo->GetActorLocation())
			<= FMath::Square(DistanciaReanimacion);
}

APersonajeLadron* APersonajeLadron::BuscarCompaneroReanimable() const
{
	if (!PuedeActuarEnPartida() || bReanimando || bRobando || bDerribado || bSiendoCargado || IsValid(CeldaActual) || GetController() == nullptr)
	{
		return nullptr;
	}

	APersonajeLadron* MasCercano = nullptr;
	float DistanciaCuadrada = FMath::Square(DistanciaReanimacion);
	for (TActorIterator<APersonajeLadron> It(GetWorld()); It; ++It)
	{
		APersonajeLadron* Candidato = *It;
		const float Distancia = FVector::DistSquared(GetActorLocation(), Candidato->GetActorLocation());
		if (Distancia <= DistanciaCuadrada && PuedeReanimar(Candidato))
		{
			MasCercano = Candidato;
			DistanciaCuadrada = Distancia;
		}
	}

	return MasCercano;
}

ACelda* APersonajeLadron::BuscarCeldaParaRescatar() const
{
	if (!PuedeActuarEnPartida() || bReanimando || bRobando || bDerribado || bSiendoCargado || IsValid(CeldaActual) || GetController() == nullptr)
	{
		return nullptr;
	}
	ACelda* Celda = ACelda::BuscarEnZona(this, true);
	return Celda && Celda->PuedeRescatar(this) ? Celda : nullptr;
}

void APersonajeLadron::Server_IniciarReanimacion_Implementation()
{
	if (bReanimando || bRobando)
	{
		return;
	}
	if (ACelda* Celda = BuscarCeldaParaRescatar())
	{
		ObjetivoCelda = Celda;
		ObjetivoReanimacion = Celda->Prisionero;
		bRescatando = true;
	}
	else if (APersonajeLadron* MasCercano = BuscarCompaneroReanimable())
	{
		ObjetivoReanimacion = MasCercano;
		ObjetivoCelda.Reset();
		bRescatando = false;
	}
	else
	{
		if (ABotin* Botin = BuscarBotinCercano())
		{
			if (!PuedeRobar())
			{
				Client_NotificarErrorInteraccion(FString::Printf(TEXT("Inventario lleno: solo puedes llevar %d objetos."), ObtenerCapacidadInventario()));
				return;
			}
			ObjetivoRobo = Botin;
			RevisionRobo = Botin->RevisionEstado;
			InicioRobo = GetWorld()->GetTimeSeconds();
			bRobando = true;
			ProgresoRobo = 0.0f;
			ForceNetUpdate();
			GetWorldTimerManager().SetTimer(TemporizadorRobo, this, &APersonajeLadron::ComprobarRobo, 0.1f, true);
		}
		return;
	}

	InicioReanimacion = GetWorld()->GetTimeSeconds();
	bReanimando = true;
	ProgresoReanimacion = 0.0f;
	ForceNetUpdate();
	GetWorldTimerManager().SetTimer(TemporizadorReanimacion, this, &APersonajeLadron::ComprobarReanimacion, 0.1f, true);
}

void APersonajeLadron::Server_CancelarReanimacion_Implementation()
{
	CancelarReanimacionServidor();
}

void APersonajeLadron::CancelarReanimacionServidor()
{
	CancelarRoboServidor();
	GetWorldTimerManager().ClearTimer(TemporizadorReanimacion);
	ObjetivoReanimacion.Reset();
	ObjetivoCelda.Reset();
	bRescatando = false;
	ProgresoReanimacion = 0.0f;
	if (bReanimando)
	{
		bReanimando = false;
		ForceNetUpdate();
	}
}

void APersonajeLadron::ComprobarReanimacion()
{
	APersonajeLadron* Objetivo = ObjetivoReanimacion.Get();
	ACelda* Celda = ObjetivoCelda.Get();
	const bool bObjetivoValido = bRescatando
		? IsValid(Celda) && Celda->Prisionero == Objetivo && Celda->PuedeRescatar(this)
		: PuedeReanimar(Objetivo);
	if (!bObjetivoValido)
	{
		CancelarReanimacionServidor();
		return;
	}

	const double TiempoTranscurrido = GetWorld()->GetTimeSeconds() - InicioReanimacion;
	ProgresoReanimacion = FMath::Clamp(static_cast<float>(TiempoTranscurrido / DuracionReanimacion), 0.0f, 1.0f);
	if (TiempoTranscurrido >= DuracionReanimacion)
	{
		const bool bEraRescate = bRescatando;
		CancelarReanimacionServidor();
		if (bEraRescate)
		{
			FString Error;
			if (!Celda->LiberarPrisionero(Error))
			{
				Client_NotificarErrorInteraccion(Error);
			}
		}
		else
		{
			Objetivo->VidasRestantes = FMath::Max(0, Objetivo->VidasRestantes - 1);
			Objetivo->RecuperarSaludYMovimiento();
		}
	}
}

void APersonajeLadron::RecuperarSaludYMovimiento()
{
	ComponenteSalud->SaludActual = ComponenteSalud->SaludMaxima;
	bDerribado = false;
	OnRep_Derribado();
	ForceNetUpdate();
}

void APersonajeLadron::Client_NotificarErrorInteraccion_Implementation(const FString& Mensaje)
{
	UE_LOG(LogTemp, Warning, TEXT("%s"), *Mensaje);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, Mensaje);
	}
}

void APersonajeLadron::EstablecerSiendoCargado(bool bCargado)
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("EstablecerSiendoCargado debe ejecutarse en el servidor."));
		return;
	}

	bSiendoCargado = bCargado && bDerribado && !IsValid(CeldaActual);
	OnRep_SiendoCargado();
	if (bSiendoCargado)
	{
		CancelarReanimacionServidor();
		for (TActorIterator<APersonajeLadron> It(GetWorld()); It; ++It)
		{
			if (It->ObjetivoReanimacion.Get() == this)
			{
				It->CancelarReanimacionServidor();
			}
		}
	}
	ForceNetUpdate();
}

void APersonajeLadron::Morir()
{
	if (AEstadoJugadorDVL* Estado = GetPlayerState<AEstadoJugadorDVL>())
	{
		Estado->bEliminado = true;
		Estado->ForceNetUpdate();
	}
	if (AModoJuegoDVL* Modo = GetWorld()->GetAuthGameMode<AModoJuegoDVL>())
	{
		Modo->EvaluarPartida();
	}
	if (APlayerController* Controlador = Cast<APlayerController>(GetController()))
	{
		if (Controlador->PlayerState)
		{
			Controlador->StartSpectatingOnly();
			Controlador->ClientGotoState(NAME_Spectating);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("No se puede activar el modo espectador de %s: falta PlayerState."), *GetName());
		}
	}

	Destroy();
}

void APersonajeLadron::AñadirBotin()
{
	UE_LOG(LogTemp, Warning, TEXT("AnadirBotin ya no incrementa el contador. Usa IniciarInteraccion para robar un objeto real."));
}

bool APersonajeLadron::PuedeRobar() const
{
	return GetController() != nullptr && !bDerribado && !bSiendoCargado && !IsValid(CeldaActual)
		&& PuedeActuarEnPartida() && !bReanimando && Inventario.Num() < ObtenerCapacidadInventario();
}

int32 APersonajeLadron::ObtenerCapacidadInventario() const
{
	const AEstadoJuegoDVL* Estado = GetWorld()->GetGameState<AEstadoJuegoDVL>();
	return Estado && Estado->bEsPartida ? Estado->CapacidadInventario : 3;
}

ABotin* APersonajeLadron::BuscarBotinCercano() const
{
	if (!PuedeActuarEnPartida() || bDerribado || bSiendoCargado || IsValid(CeldaActual) || !GetController() || bReanimando || bRobando)
	{
		return nullptr;
	}
	ABotin* MasCercano = nullptr;
	double MejorDistancia = TNumericLimits<double>::Max();
	for (TActorIterator<ABotin> It(GetWorld()); It; ++It)
	{
		const double Distancia = FVector::DistSquared(GetActorLocation(), It->GetActorLocation());
		if (It->EstaAlAlcance(this) && Distancia < MejorDistancia)
		{
			MasCercano = *It;
			MejorDistancia = Distancia;
		}
	}
	return MasCercano;
}

void APersonajeLadron::CancelarRoboServidor()
{
	GetWorldTimerManager().ClearTimer(TemporizadorRobo);
	bRobando = false;
	ObjetivoRobo = nullptr;
	ProgresoRobo = 0.0f;
	ForceNetUpdate();
}

void APersonajeLadron::ComprobarRobo()
{
	ABotin* Botin = ObjetivoRobo;
	if (!PuedeRobar() || !IsValid(Botin) || !Botin->EstaAlAlcance(this) || Botin->RevisionEstado != RevisionRobo)
	{
		CancelarRoboServidor();
		return;
	}
	const double Transcurrido = GetWorld()->GetTimeSeconds() - InicioRobo;
	ProgresoRobo = FMath::Clamp(static_cast<float>(Transcurrido / DuracionRobo), 0.0f, 1.0f);
	if (Transcurrido >= DuracionRobo)
	{
		if (Botin->Recoger(this))
		{
			Inventario.Add(Botin);
			BotinRecolectado = Inventario.Num();
			ActualizarResumenInventario();
			if (AModoJuegoDVL* Modo = GetWorld()->GetAuthGameMode<AModoJuegoDVL>())
			{
				Modo->AvisarRobo(Botin);
				Modo->EvaluarPartida();
			}
		}
		else
		{
			Client_NotificarErrorInteraccion(TEXT("No se pudo recoger el objeto; puede que otro ladron lo haya recogido."));
		}
		CancelarRoboServidor();
	}
}

void APersonajeLadron::SoltarInventario()
{
	const TArray<TObjectPtr<ABotin>> Objetos = Inventario;
	Inventario.Reset();
	BotinRecolectado = 0;
	ActualizarResumenInventario();
	for (int32 Indice = 0; Indice < Objetos.Num(); ++Indice)
	{
		if (IsValid(Objetos[Indice]))
		{
			Objetos[Indice]->Soltar(this, Indice);
		}
	}
	ForceNetUpdate();
}

void APersonajeLadron::ActualizarResumenInventario()
{
	if (AEstadoJugadorDVL* Estado = GetPlayerState<AEstadoJugadorDVL>())
	{
		Estado->ObjetosRobados.Reset();
		for (const ABotin* Objeto : Inventario)
		{
			if (IsValid(Objeto))
			{
				Estado->ObjetosRobados.Add(Objeto->NombreObjeto);
			}
		}
		Estado->ForceNetUpdate();
	}
}
