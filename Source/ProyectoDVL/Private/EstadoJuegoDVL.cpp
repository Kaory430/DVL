// Fill out your copyright notice in the Description page of Project Settings.


#include "EstadoJuegoDVL.h"

#include "Net/UnrealNetwork.h"

int32 AEstadoJuegoDVL::ContarBando(EBandoDVL Bando) const
{
	int32 Cantidad = 0;
	for (const APlayerState* Jugador : PlayerArray)
	{
		const AEstadoJugadorDVL* Estado = Cast<AEstadoJugadorDVL>(Jugador);
		if (IsValid(Estado) && !Estado->IsOnlyASpectator() && Estado->Bando == Bando)
		{
			++Cantidad;
		}
	}
	return Cantidad;
}

bool AEstadoJuegoDVL::PuedeElegirBando(const AEstadoJugadorDVL* Jugador, EBandoDVL Bando, FString& Error) const
{
	if (!bEsSala || bIniciando || !IsValid(Jugador) || !PlayerArray.Contains(Jugador))
	{
		Error = TEXT("No puedes cambiar de bando en este momento.");
		return false;
	}
	if (Bando != EBandoDVL::Policia && Bando != EBandoDVL::Ladron)
	{
		Error = TEXT("El bando solicitado no es valido.");
		return false;
	}
	const int32 Limite = Bando == EBandoDVL::Policia ? 1 : 4;
	if (Jugador->Bando != Bando && ContarBando(Bando) >= Limite)
	{
		Error = TEXT("Ese bando ya esta completo. Elige el otro.");
		return false;
	}
	return true;
}

bool AEstadoJuegoDVL::PuedeIniciar(FString& Error) const
{
	if (!bEsSala || PlayerArray.Num() > 5)
	{
		Error = TEXT("La sala no esta disponible o supera los cinco jugadores.");
		return false;
	}
	if (ContarBando(EBandoDVL::Policia) != 1 || ContarBando(EBandoDVL::Ladron) < 2
		|| ContarBando(EBandoDVL::Ladron) > 4)
	{
		Error = TEXT("Se necesita 1 policia y entre 2 y 4 ladrones.");
		return false;
	}
	for (const APlayerState* Jugador : PlayerArray)
	{
		const AEstadoJugadorDVL* Estado = Cast<AEstadoJugadorDVL>(Jugador);
		if (!IsValid(Estado) || Estado->Bando == EBandoDVL::SinElegir || !Estado->bListo)
		{
			Error = TEXT("Todos deben elegir bando y marcarse listos.");
			return false;
		}
	}
	return true;
}

int32 AEstadoJuegoDVL::SegundosRestantes() const
{
	return Fase == EFasePartidaDVL::EnCurso
		? FMath::Max(0, FMath::CeilToInt(FinPartida - GetServerWorldTimeSeconds())) : 0;
}

bool AEstadoJuegoDVL::PermiteAcciones() const
{
	return !bEsPartida || (Fase == EFasePartidaDVL::EnCurso && GetServerWorldTimeSeconds() < FinPartida);
}

bool AEstadoJuegoDVL::ComenzarRonda(FString& Error)
{
	if (!HasAuthority() || !bEsPartida || Fase != EFasePartidaDVL::Esperando
		|| PlayerArray.Num() < 3 || PlayerArray.Num() > 5
		|| ContarBando(EBandoDVL::Policia) != 1 || ContarBando(EBandoDVL::Ladron) != PlayerArray.Num() - 1)
	{
		Error = TEXT("La ronda necesita entre 3 y 5 jugadores: un policia y al menos dos ladrones.");
		return false;
	}
	CapacidadInventario = PlayerArray.Num() < 4 ? 3 : 2;
	FinPartida = GetServerWorldTimeSeconds() + 900.0;
	Fase = EFasePartidaDVL::EnCurso;
	ForceNetUpdate();
	return true;
}

EBandoDVL AEstadoJuegoDVL::CalcularGanador(int32 ObjetosEnMochilas, int32 LadronesVivos) const
{
	if (Fase != EFasePartidaDVL::EnCurso)
	{
		return EBandoDVL::SinElegir;
	}
	if (GetServerWorldTimeSeconds() >= FinPartida || LadronesVivos == 0)
	{
		return EBandoDVL::Policia;
	}
	return ObjetosEnMochilas >= 5 ? EBandoDVL::Ladron : EBandoDVL::SinElegir;
}

void AEstadoJuegoDVL::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEstadoJuegoDVL, bEsSala);
	DOREPLIFETIME(AEstadoJuegoDVL, bIniciando);
	DOREPLIFETIME(AEstadoJuegoDVL, Anfitrion);
	DOREPLIFETIME(AEstadoJuegoDVL, bEsPartida);
	DOREPLIFETIME(AEstadoJuegoDVL, Fase);
	DOREPLIFETIME(AEstadoJuegoDVL, FinPartida);
	DOREPLIFETIME(AEstadoJuegoDVL, ArtefactosRobados);
	DOREPLIFETIME(AEstadoJuegoDVL, CapacidadInventario);
	DOREPLIFETIME(AEstadoJuegoDVL, BandoGanador);
	DOREPLIFETIME(AEstadoJuegoDVL, MotivoResultado);
}
