#include "ModoSalaDVL.h"

#include "ControladorJugadorDVL.h"
#include "Engine/World.h"
#include "EstadoJuegoDVL.h"
#include "GameFramework/GameSession.h"
#include "InstanciaJuegoDVL.h"
#include "Misc/PackageName.h"
#include "PersonajeLadron.h"
#include "PersonajePolicia.h"

AModoSalaDVL::AModoSalaDVL()
{
	bUseSeamlessTravel = true;
}

void AModoSalaDVL::InitGameState()
{
	Super::InitGameState();
	GetGameState<AEstadoJuegoDVL>()->bEsSala = true;
}

void AModoSalaDVL::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	const AEstadoJuegoDVL* Estado = GetGameState<AEstadoJuegoDVL>();
	if (ErrorMessage.IsEmpty() && (!Estado || Estado->bIniciando || GetNumPlayers() >= 5))
	{
		ErrorMessage = TEXT("La sala esta llena o la partida esta iniciando.");
	}
}

void AModoSalaDVL::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	if (GetNumPlayers() > 5)
	{
		GameSession->KickPlayer(NewPlayer, FText::FromString(TEXT("La sala ya tiene cinco jugadores.")));
		return;
	}
	AEstadoJuegoDVL* Estado = GetGameState<AEstadoJuegoDVL>();
	if (NewPlayer->IsLocalController())
	{
		Estado->Anfitrion = NewPlayer->GetPlayerState<AEstadoJugadorDVL>();
		Estado->ForceNetUpdate();
	}
}

void AModoSalaDVL::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
	if (AEstadoJugadorDVL* Jugador = NewPlayer->GetPlayerState<AEstadoJugadorDVL>())
	{
		Jugador->Bando = EBandoDVL::SinElegir;
		Jugador->bListo = false;
		Jugador->bEliminado = false;
		Jugador->bQuiereRevancha = false;
		Jugador->ObjetosRobados.Reset();
		Jugador->SetIsOnlyASpectator(false);
		Jugador->SetIsSpectator(false);
		Jugador->ForceNetUpdate();
	}
	if (NewPlayer->IsLocalController())
	{
		AEstadoJuegoDVL* Estado = GetGameState<AEstadoJuegoDVL>();
		Estado->Anfitrion = NewPlayer->GetPlayerState<AEstadoJugadorDVL>();
		Estado->ForceNetUpdate();
	}
}

void AModoSalaDVL::Logout(AController* Exiting)
{
	AEstadoJugadorDVL* Jugador = Exiting ? Exiting->GetPlayerState<AEstadoJugadorDVL>() : nullptr;
	Super::Logout(Exiting);
	if (AEstadoJuegoDVL* Estado = GetGameState<AEstadoJuegoDVL>())
	{
		if (Jugador)
		{
			Estado->RemovePlayerState(Jugador);
		}
		Estado->ForceNetUpdate();
	}
}

void AModoSalaDVL::ElegirBando(AControladorJugadorDVL* Jugador, EBandoDVL Bando)
{
	AEstadoJuegoDVL* Sala = GetGameState<AEstadoJuegoDVL>();
	AEstadoJugadorDVL* Estado = Jugador ? Jugador->GetPlayerState<AEstadoJugadorDVL>() : nullptr;
	if (!Sala || Sala->bIniciando || !Estado)
	{
		return;
	}
	FString Error;
	if (!Sala->PuedeElegirBando(Estado, Bando, Error))
	{
		Jugador->Client_MensajeSala(Error);
		return;
	}
	if (Estado->Bando == Bando)
	{
		return;
	}
	Estado->Bando = Bando;
	Estado->bListo = false;
	Estado->ForceNetUpdate();
}

void AModoSalaDVL::CambiarListo(AControladorJugadorDVL* Jugador, bool bListo)
{
	const AEstadoJuegoDVL* Sala = GetGameState<AEstadoJuegoDVL>();
	AEstadoJugadorDVL* Estado = Jugador ? Jugador->GetPlayerState<AEstadoJugadorDVL>() : nullptr;
	if (!Sala || Sala->bIniciando || !Estado)
	{
		return;
	}
	if (Estado->Bando == EBandoDVL::SinElegir)
	{
		Jugador->Client_MensajeSala(TEXT("Primero elige un bando."));
		return;
	}
	Estado->bListo = bListo;
	Estado->ForceNetUpdate();
}

void AModoSalaDVL::SolicitarInicio(AControladorJugadorDVL* Jugador)
{
	AEstadoJuegoDVL* Sala = GetGameState<AEstadoJuegoDVL>();
	if (!Sala || Sala->bIniciando || !Jugador)
	{
		return;
	}
	FString Error;
	if (Sala->Anfitrion != Jugador->PlayerState || !Sala->PuedeIniciar(Error))
	{
		Jugador->Client_MensajeSala(Error.IsEmpty() ? TEXT("Solo el anfitrion puede iniciar.") : Error);
		return;
	}
	UInstanciaJuegoDVL* Instancia = GetGameInstance<UInstanciaJuegoDVL>();
	if (!Instancia || Instancia->bOcupado)
	{
		Jugador->Client_MensajeSala(TEXT("No esta disponible la sesion. Espera a que termine la operacion actual."));
		return;
	}
	if (!Instancia->ValidarViaje(false, Error))
	{
		Jugador->Client_MensajeSala(Error);
		return;
	}
	UClass* Policia = Instancia->ClasePolicia.LoadSynchronous();
	UClass* Ladron = Instancia->ClaseLadron.LoadSynchronous();
	const FString Mapa = Instancia->MapaJuego.ToSoftObjectPath().GetLongPackageName();
	if (!Policia || !Ladron || !FPackageName::DoesPackageExist(Mapa))
	{
		Jugador->Client_MensajeSala(TEXT("Falta el mapa de juego o una clase de personaje en InstanciaJuegoDVL."));
		return;
	}
	Sala->bIniciando = true;
	Sala->ForceNetUpdate();
	Instancia->CerrarSalaEIniciar();
}

bool AModoSalaDVL::IniciarViaje(FString& Error)
{
	AEstadoJuegoDVL* Sala = GetGameState<AEstadoJuegoDVL>();
	UInstanciaJuegoDVL* Instancia = GetGameInstance<UInstanciaJuegoDVL>();
	if (!Sala || !Instancia || !Sala->PuedeIniciar(Error))
	{
		if (Error.IsEmpty())
		{
			Error = TEXT("La sala ya no esta disponible.");
		}
		return false;
	}
	Instancia->JugadoresEsperados = Sala->PlayerArray.Num();
	return Instancia->ViajarConSesion(false, Error);
}
