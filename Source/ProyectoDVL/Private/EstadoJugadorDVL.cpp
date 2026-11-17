// Fill out your copyright notice in the Description page of Project Settings.


#include "EstadoJugadorDVL.h"

#include "Net/UnrealNetwork.h"

void AEstadoJugadorDVL::CopyProperties(APlayerState* NuevoEstado)
{
	Super::CopyProperties(NuevoEstado);
	if (AEstadoJugadorDVL* Estado = Cast<AEstadoJugadorDVL>(NuevoEstado))
	{
		Estado->Bando = Bando;
		Estado->bListo = bListo;
	}
}

void AEstadoJugadorDVL::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEstadoJugadorDVL, Bando);
	DOREPLIFETIME(AEstadoJugadorDVL, bListo);
	DOREPLIFETIME(AEstadoJugadorDVL, ObjetosRobados);
	DOREPLIFETIME(AEstadoJugadorDVL, bEliminado);
	DOREPLIFETIME(AEstadoJugadorDVL, bQuiereRevancha);
}
