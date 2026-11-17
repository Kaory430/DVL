#include "ModoMenuDVL.h"

#include "ControladorJugadorDVL.h"
#include "EstadoJuegoDVL.h"
#include "EstadoJugadorDVL.h"

AModoMenuDVL::AModoMenuDVL()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AControladorJugadorDVL::StaticClass();
	PlayerStateClass = AEstadoJugadorDVL::StaticClass();
	GameStateClass = AEstadoJuegoDVL::StaticClass();
}

void AModoMenuDVL::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// El menu y la sala no deben crear personajes jugables.
}
