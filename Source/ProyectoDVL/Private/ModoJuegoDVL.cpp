// Fill out your copyright notice in the Description page of Project Settings.


#include "ModoJuegoDVL.h"

#include "ControladorJugadorDVL.h"
#include "Botin.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "EstadoJuegoDVL.h"
#include "EstadoJugadorDVL.h"
#include "InstanciaJuegoDVL.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PersonajeLadron.h"
#include "PersonajePolicia.h"
#include "PowerupComponent.h"

AModoJuegoDVL::AModoJuegoDVL()
{
	PlayerControllerClass = AControladorJugadorDVL::StaticClass();
	PlayerStateClass = AEstadoJugadorDVL::StaticClass();
	GameStateClass = AEstadoJuegoDVL::StaticClass();
	bUseSeamlessTravel = true;
}

void AModoJuegoDVL::InitGameState()
{
	Super::InitGameState();
	GetGameState<AEstadoJuegoDVL>()->bEsPartida = true;
}

void AModoJuegoDVL::BeginPlay()
{
	Super::BeginPlay();
	LimiteCarga = GetWorld()->GetTimeSeconds() + 60.0;
	GetWorldTimerManager().SetTimer(TemporizadorPartida, this, &AModoJuegoDVL::EvaluarPartida, 0.2f, true);
}

void AModoJuegoDVL::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bCerrando = true;
	GetWorldTimerManager().ClearTimer(TemporizadorPartida);
	Super::EndPlay(EndPlayReason);
}

void AModoJuegoDVL::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
	if (APersonajeBase* Personaje = Cast<APersonajeBase>(NewPlayer->GetPawn()))
	{
		if (GetGameState<AEstadoJuegoDVL>()->Fase == EFasePartidaDVL::Esperando)
		{
			Personaje->GetCharacterMovement()->DisableMovement();
		}
	}
	if (NewPlayer->IsLocalController())
	{
		AEstadoJuegoDVL* Estado = GetGameState<AEstadoJuegoDVL>();
		Estado->Anfitrion = NewPlayer->GetPlayerState<AEstadoJugadorDVL>();
		Estado->ForceNetUpdate();
	}
}

void AModoJuegoDVL::Logout(AController* Exiting)
{
	APlayerState* Saliente = Exiting ? Exiting->PlayerState : nullptr;
	Super::Logout(Exiting);
	AEstadoJuegoDVL* Estado = GetGameState<AEstadoJuegoDVL>();
	if (Estado && !bCerrando && Estado->Fase != EFasePartidaDVL::Viajando)
	{
		if (Saliente)
		{
			Estado->RemovePlayerState(Saliente);
		}
		VolverASala();
	}
}

void AModoJuegoDVL::EvaluarPartida()
{
	AEstadoJuegoDVL* Estado = GetGameState<AEstadoJuegoDVL>();
	if (!Estado || bCerrando || Estado->Fase == EFasePartidaDVL::Finalizada || Estado->Fase == EFasePartidaDVL::Viajando)
	{
		return;
	}
	if (Estado->Fase == EFasePartidaDVL::Esperando)
	{
		const UInstanciaJuegoDVL* Instancia = GetGameInstance<UInstanciaJuegoDVL>();
		const int32 Esperados = Instancia ? FMath::Max(3, Instancia->JugadoresEsperados) : 3;
		bool bPreparados = Estado->PlayerArray.Num() == Esperados;
		for (const APlayerState* Jugador : Estado->PlayerArray)
		{
			bPreparados &= Jugador && Jugador->GetPawn() != nullptr;
		}
		if (!bPreparados)
		{
			if (GetWorld()->GetTimeSeconds() >= LimiteCarga)
			{
				Finalizar(EBandoDVL::SinElegir, TEXT("No todos los jugadores pudieron entrar al mapa. Regresen a la sala."));
			}
			return;
		}
		int32 Objetos = 0;
		for (TActorIterator<ABotin> It(GetWorld()); It; ++It)
		{
			++Objetos;
		}
		if (Objetos != 5)
		{
			Finalizar(EBandoDVL::SinElegir, FString::Printf(TEXT("El mapa necesita exactamente 5 actores de botin; se encontraron %d. Corrige el mapa."), Objetos));
			return;
		}
		FString Error;
		if (!Estado->ComenzarRonda(Error))
		{
			Finalizar(EBandoDVL::SinElegir, Error);
			return;
		}
		for (TActorIterator<APersonajeBase> It(GetWorld()); It; ++It)
		{
			It->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		}
	}

	int32 Vivos = 0;
	for (const APlayerState* Jugador : Estado->PlayerArray)
	{
		const AEstadoJugadorDVL* Participante = Cast<AEstadoJugadorDVL>(Jugador);
		if (!Participante || Participante->Bando != EBandoDVL::Ladron)
		{
			continue;
		}
		Vivos += Participante->bEliminado ? 0 : 1;
	}
	Estado->ArtefactosRobados = ContarArtefactosEnMochilas();
	const EBandoDVL Ganador = Estado->CalcularGanador(Estado->ArtefactosRobados, Vivos);
	if (Ganador != EBandoDVL::SinElegir)
	{
		const FString Motivo = Ganador == EBandoDVL::Ladron ? TEXT("Los ladrones reunieron los 5 artefactos.")
			: Vivos == 0 ? TEXT("Todos los ladrones fueron eliminados definitivamente.") : TEXT("Se agotaron los 15 minutos.");
		Finalizar(Ganador, Motivo);
	}
}

int32 AModoJuegoDVL::ContarArtefactosEnMochilas() const
{
	TSet<const ABotin*> ObjetosUnicos;
	for (const APlayerState* Jugador : GetGameState<AEstadoJuegoDVL>()->PlayerArray)
	{
		const AEstadoJugadorDVL* Participante = Cast<AEstadoJugadorDVL>(Jugador);
		if (Participante && Participante->Bando == EBandoDVL::Ladron)
		{
			if (const APersonajeLadron* Ladron = Cast<APersonajeLadron>(Participante->GetPawn()))
			{
				for (const ABotin* Objeto : Ladron->Inventario)
				{
					if (IsValid(Objeto) && Objeto->Estado == EEstadoBotin::Transportado)
					{
						ObjetosUnicos.Add(Objeto);
					}
				}
			}
		}
	}
	return ObjetosUnicos.Num();
}

void AModoJuegoDVL::EnviarAviso(const FText& Mensaje, EBandoDVL Destinatarios)
{
	const AEstadoJuegoDVL* Estado = GetGameState<AEstadoJuegoDVL>();
	if (!HasAuthority() || !Estado || !Estado->PermiteAcciones())
	{
		return;
	}
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AControladorJugadorDVL* Controlador = Cast<AControladorJugadorDVL>(It->Get());
		const AEstadoJugadorDVL* Jugador = Controlador ? Controlador->GetPlayerState<AEstadoJugadorDVL>() : nullptr;
		if (Jugador && (Destinatarios == EBandoDVL::SinElegir || Jugador->Bando == Destinatarios))
		{
			Controlador->Client_AvisoPartida(Mensaje);
		}
	}
}

void AModoJuegoDVL::AvisarRobo(const ABotin* Objeto)
{
	if (!ensure(IsValid(Objeto)))
	{
		return;
	}
	const int32 Faltan = FMath::Max(0, 5 - ContarArtefactosEnMochilas());
	EnviarAviso(FText::Format(NSLOCTEXT("AvisosDVL", "RoboPolicia",
		"Los ladrones robaron {0}. Les faltan {1} artefactos por reunir."),
		Objeto->NombreObjeto, FText::AsNumber(Faltan)), EBandoDVL::Policia);
	EnviarAviso(FText::Format(NSLOCTEXT("AvisosDVL", "RoboLadrones",
		"Tu equipo robo {0}. Faltan {1} artefactos por reunir."),
		Objeto->NombreObjeto, FText::AsNumber(Faltan)), EBandoDVL::Ladron);
}

void AModoJuegoDVL::AvisarDevolucion(const ABotin* Objeto)
{
	if (ensure(IsValid(Objeto)))
	{
		EnviarAviso(FText::Format(NSLOCTEXT("AvisosDVL", "Devolucion",
			"El policia devolvio {0} a su lugar."), Objeto->NombreObjeto), EBandoDVL::SinElegir);
	}
}

void AModoJuegoDVL::AvisarDerribo(const APersonajeLadron* Ladron, bool bPorPolicia)
{
	const APlayerState* Jugador = IsValid(Ladron) ? Ladron->GetPlayerState() : nullptr;
	if (!Jugador)
	{
		UE_LOG(LogTemp, Warning, TEXT("No se pudo anunciar el derribo: falta el jugador."));
		return;
	}
	EnviarAviso(FText::Format(bPorPolicia
		? NSLOCTEXT("AvisosDVL", "DerriboPolicial", "El policia ha derribado a {0}.")
		: NSLOCTEXT("AvisosDVL", "Derribo", "{0} ha sido derribado."),
		FText::FromString(Jugador->GetPlayerName())), EBandoDVL::Ladron);
}

void AModoJuegoDVL::AvisarPulso()
{
	EnviarAviso(NSLOCTEXT("PowerupsDVL", "AdvertenciaPulso", "El policia activo un pulso de busqueda. Puede detectar ladrones cercanos durante 5 segundos."),
		EBandoDVL::Ladron);
}

void AModoJuegoDVL::Finalizar(EBandoDVL Ganador, const FString& Motivo)
{
	AEstadoJuegoDVL* Estado = GetGameState<AEstadoJuegoDVL>();
	Estado->Fase = EFasePartidaDVL::Finalizada;
	Estado->BandoGanador = Ganador;
	Estado->MotivoResultado = Motivo;
	Estado->ForceNetUpdate();
	UE_LOG(LogTemp, Display, TEXT("Fin de ronda DVL: %s"), *Motivo);
	for (TActorIterator<APersonajeBase> It(GetWorld()); It; ++It)
	{
		It->ComponentePowerup->CancelarEfecto();
		It->GetCharacterMovement()->StopMovementImmediately();
		It->GetCharacterMovement()->DisableMovement();
		if (APersonajeLadron* Ladron = Cast<APersonajeLadron>(*It))
		{
			Ladron->CancelarInteraccion();
		}
	}
}

void AModoJuegoDVL::VotarRevancha(APlayerController* Jugador, bool bAceptar)
{
	AEstadoJuegoDVL* Estado = GetGameState<AEstadoJuegoDVL>();
	AEstadoJugadorDVL* Participante = Jugador ? Jugador->GetPlayerState<AEstadoJugadorDVL>() : nullptr;
	if (!Estado || Estado->Fase != EFasePartidaDVL::Finalizada || Estado->BandoGanador == EBandoDVL::SinElegir
		|| !Participante || !Estado->PlayerArray.Contains(Participante))
	{
		return;
	}
	if (!bAceptar)
	{
		VolverASala();
		return;
	}
	Participante->bQuiereRevancha = true;
	Participante->ForceNetUpdate();
	for (const APlayerState* Otro : Estado->PlayerArray)
	{
		const AEstadoJugadorDVL* Votante = Cast<AEstadoJugadorDVL>(Otro);
		if (!Votante || !Votante->bQuiereRevancha)
		{
			return;
		}
	}
	ReiniciarRonda();
}

void AModoJuegoDVL::SolicitarVolverASala(APlayerController* Jugador)
{
	const AEstadoJuegoDVL* Estado = GetGameState<AEstadoJuegoDVL>();
	if (Jugador && Estado && Jugador->PlayerState == Estado->Anfitrion)
	{
		VolverASala();
	}
}

void AModoJuegoDVL::PrepararJugadoresParaViaje(bool bBorrarBandos)
{
	AEstadoJuegoDVL* Estado = GetGameState<AEstadoJuegoDVL>();
	for (APlayerState* Jugador : Estado->PlayerArray)
	{
		if (AEstadoJugadorDVL* Participante = Cast<AEstadoJugadorDVL>(Jugador))
		{
			if (bBorrarBandos)
			{
				Participante->Bando = EBandoDVL::SinElegir;
			}
			Participante->bListo = false;
			Participante->bEliminado = false;
			Participante->bQuiereRevancha = false;
			Participante->ObjetosRobados.Reset();
			Participante->SetIsOnlyASpectator(false);
			Participante->SetIsSpectator(false);
			Participante->ForceNetUpdate();
		}
	}
}

void AModoJuegoDVL::ReiniciarRonda()
{
	AEstadoJuegoDVL* Estado = GetGameState<AEstadoJuegoDVL>();
	UInstanciaJuegoDVL* Instancia = GetGameInstance<UInstanciaJuegoDVL>();
	FString Error;
	if (!Instancia || !Instancia->ValidarViaje(false, Error))
	{
		Finalizar(EBandoDVL::SinElegir, Error.IsEmpty() ? TEXT("No se pudo preparar la revancha.") : Error);
		return;
	}
	Estado->Fase = EFasePartidaDVL::Viajando;
	PrepararJugadoresParaViaje(false);
	Instancia->JugadoresEsperados = Estado->PlayerArray.Num();
	if (!Instancia->ViajarConSesion(false, Error))
	{
		Finalizar(EBandoDVL::SinElegir, Error);
	}
}

void AModoJuegoDVL::VolverASala()
{
	AEstadoJuegoDVL* Estado = GetGameState<AEstadoJuegoDVL>();
	if (Estado->Fase == EFasePartidaDVL::Viajando)
	{
		return;
	}
	UInstanciaJuegoDVL* Instancia = GetGameInstance<UInstanciaJuegoDVL>();
	FString Error;
	if (!Instancia || !Instancia->ValidarViaje(true, Error))
	{
		Finalizar(EBandoDVL::SinElegir, Error.IsEmpty() ? TEXT("No se pudo preparar el regreso a la sala.") : Error);
		return;
	}
	Estado->Fase = EFasePartidaDVL::Viajando;
	Estado->ForceNetUpdate();
	PrepararJugadoresParaViaje(true);
	if (!Instancia->ReabrirSala())
	{
		Finalizar(EBandoDVL::SinElegir, TEXT("No se pudo reabrir la sala. Revisa el mensaje de conexion."));
	}
}
void AModoJuegoDVL::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (ErrorMessage.IsEmpty())
	{
		ErrorMessage = TEXT("La partida ya comenzo. Espera una sala nueva.");
	}
}

UClass* AModoJuegoDVL::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	const AEstadoJugadorDVL* Estado = InController ? InController->GetPlayerState<AEstadoJugadorDVL>() : nullptr;
	UInstanciaJuegoDVL* Instancia = GetGameInstance<UInstanciaJuegoDVL>();
	UClass* Clase = nullptr;
	if (Estado && Instancia)
	{
		if (Estado->Bando == EBandoDVL::Policia)
		{
			Clase = Instancia->ClasePolicia.LoadSynchronous();
		}
		else if (Estado->Bando == EBandoDVL::Ladron)
		{
			Clase = Instancia->ClaseLadron.LoadSynchronous();
		}
	}
	if (!Clase)
	{
		UE_LOG(LogTemp, Error, TEXT("No se pudo resolver el personaje del jugador %s. Revisa las clases en InstanciaJuegoDVL."), *GetNameSafe(InController));
	}
	return Clase;
}
