#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EstadoJuegoDVL.h"
#include "EstadoJugadorDVL.h"

namespace
{
	struct FSalaPrueba
	{
		UWorld* Mundo;
		AEstadoJuegoDVL* Sala;

		FSalaPrueba()
		{
			Mundo = UWorld::CreateWorld(EWorldType::Game, false);
			Sala = Mundo->SpawnActor<AEstadoJuegoDVL>();
			Sala->bEsSala = true;
		}

		~FSalaPrueba()
		{
			Mundo->DestroyWorld(false);
		}

		AEstadoJugadorDVL* Agregar(EBandoDVL Bando = EBandoDVL::SinElegir, bool bListo = false)
		{
			AEstadoJugadorDVL* Jugador = Mundo->SpawnActor<AEstadoJugadorDVL>();
			Jugador->Bando = Bando;
			Jugador->bListo = bListo;
			Sala->AddPlayerState(Jugador);
			return Jugador;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCuposSalaDVLTest, "ProyectoDVL.Sala.CuposAutoritativos",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCuposSalaDVLTest::RunTest(const FString& Parameters)
{
	FSalaPrueba Prueba;
	AEstadoJugadorDVL* Primero = Prueba.Agregar();
	AEstadoJugadorDVL* Segundo = Prueba.Agregar();
	FString Error;
	TestTrue(TEXT("El primer jugador puede elegir policia"), Prueba.Sala->PuedeElegirBando(Primero, EBandoDVL::Policia, Error));
	Primero->Bando = EBandoDVL::Policia;
	TestFalse(TEXT("El segundo no puede ocupar la plaza ya tomada"), Prueba.Sala->PuedeElegirBando(Segundo, EBandoDVL::Policia, Error));
	TestTrue(TEXT("El segundo puede ser ladron"), Prueba.Sala->PuedeElegirBando(Segundo, EBandoDVL::Ladron, Error));
	Segundo->Bando = EBandoDVL::Ladron;
	Prueba.Agregar(EBandoDVL::Ladron);
	Prueba.Agregar(EBandoDVL::Ladron);
	Prueba.Agregar(EBandoDVL::Ladron);
	TestFalse(TEXT("El policia no puede cambiar a un quinto ladron"), Prueba.Sala->PuedeElegirBando(Primero, EBandoDVL::Ladron, Error));
	TestFalse(TEXT("No admite bandos sin elegir como seleccion"), Prueba.Sala->PuedeElegirBando(Primero, EBandoDVL::SinElegir, Error));
	Prueba.Sala->RemovePlayerState(Primero);
	TestTrue(TEXT("La plaza se libera al salir su ocupante"), Prueba.Sala->PuedeElegirBando(Segundo, EBandoDVL::Policia, Error));
	Prueba.Sala->bIniciando = true;
	TestFalse(TEXT("No se cambia de bando durante el viaje"), Prueba.Sala->PuedeElegirBando(Segundo, EBandoDVL::Policia, Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInicioSalaDVLTest, "ProyectoDVL.Sala.InicioYViaje",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInicioSalaDVLTest::RunTest(const FString& Parameters)
{
	FSalaPrueba Prueba;
	FString Error;
	AEstadoJugadorDVL* Policia = Prueba.Agregar(EBandoDVL::Policia, true);
	TestFalse(TEXT("No inicia solo con policia"), Prueba.Sala->PuedeIniciar(Error));
	AEstadoJugadorDVL* Ladron = Prueba.Agregar(EBandoDVL::Ladron);
	TestFalse(TEXT("No inicia si falta listo"), Prueba.Sala->PuedeIniciar(Error));
	Ladron->bListo = true;
	TestFalse(TEXT("No permite iniciar con un solo ladron aunque todos esten listos"), Prueba.Sala->PuedeIniciar(Error));
	Prueba.Agregar(EBandoDVL::Ladron, true);
	TestTrue(TEXT("Permite iniciar con un policia y dos ladrones listos"), Prueba.Sala->PuedeIniciar(Error));
	Prueba.Agregar(EBandoDVL::Ladron, true);
	Prueba.Agregar(EBandoDVL::Ladron, true);
	TestTrue(TEXT("Permite 1 policia y 4 ladrones listos"), Prueba.Sala->PuedeIniciar(Error));
	AEstadoJugadorDVL* Extra = Prueba.Agregar();
	TestFalse(TEXT("Rechaza un sexto jugador"), Prueba.Sala->PuedeIniciar(Error));
	Prueba.Sala->RemovePlayerState(Extra);

	AEstadoJugadorDVL* Destino = Prueba.Mundo->SpawnActor<AEstadoJugadorDVL>();
	Policia->CopyProperties(Destino);
	TestEqual(TEXT("El bando se conserva al copiar para seamless travel"), Destino->Bando, EBandoDVL::Policia);
	TestTrue(TEXT("El estado listo tambien se conserva"), Destino->bListo);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FResultadoRondaDVLTest, "ProyectoDVL.Partida.ResultadoYTiempo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FResultadoRondaDVLTest::RunTest(const FString& Parameters)
{
	FSalaPrueba Prueba;
	Prueba.Sala->bEsSala = false;
	Prueba.Sala->bEsPartida = true;
	TestFalse(TEXT("No permite robar mientras se prepara la ronda"), Prueba.Sala->PermiteAcciones());
	FString Error;
	Prueba.Agregar(EBandoDVL::Policia);
	Prueba.Agregar(EBandoDVL::Ladron);
	TestFalse(TEXT("No puede comenzar con un solo ladron"), Prueba.Sala->ComenzarRonda(Error));
	Prueba.Agregar(EBandoDVL::Ladron);
	TestTrue(TEXT("Comienza con tres jugadores"), Prueba.Sala->ComenzarRonda(Error));
	TestEqual(TEXT("Con tres jugadores cada ladron lleva tres objetos"), Prueba.Sala->CapacidadInventario, 3);
	TestEqual(TEXT("El reloj comienza con quince minutos"), Prueba.Sala->SegundosRestantes(), 900);
	TestTrue(TEXT("Permite acciones antes del limite"), Prueba.Sala->PermiteAcciones());
	TestTrue(TEXT("Cuatro artefactos no ganan"), Prueba.Sala->CalcularGanador(4, 2) == EBandoDVL::SinElegir);
	TestTrue(TEXT("Cinco artefactos en mochilas ganan"), Prueba.Sala->CalcularGanador(5, 2) == EBandoDVL::Ladron);
	TestTrue(TEXT("La eliminacion definitiva de todos da victoria policial"), Prueba.Sala->CalcularGanador(0, 0) == EBandoDVL::Policia);
	Prueba.Sala->FinPartida = Prueba.Sala->GetServerWorldTimeSeconds();
	TestFalse(TEXT("El limite exacto ya no permite acciones"), Prueba.Sala->PermiteAcciones());
	TestTrue(TEXT("El tiempo agotado gana incluso con una recogida tardia"), Prueba.Sala->CalcularGanador(5, 2) == EBandoDVL::Policia);
	Prueba.Sala->Fase = EFasePartidaDVL::Finalizada;
	TestTrue(TEXT("El resultado no se calcula otra vez al finalizar"), Prueba.Sala->CalcularGanador(5, 2) == EBandoDVL::SinElegir);
	Prueba.Agregar(EBandoDVL::Ladron);
	Prueba.Sala->Fase = EFasePartidaDVL::Esperando;
	TestTrue(TEXT("Comienza con cuatro jugadores"), Prueba.Sala->ComenzarRonda(Error));
	TestEqual(TEXT("Con cuatro jugadores la capacidad baja a dos"), Prueba.Sala->CapacidadInventario, 2);
	Prueba.Agregar(EBandoDVL::Ladron);
	Prueba.Sala->Fase = EFasePartidaDVL::Esperando;
	TestTrue(TEXT("Comienza con cinco jugadores"), Prueba.Sala->ComenzarRonda(Error));
	TestEqual(TEXT("Con cinco jugadores la capacidad sigue en dos"), Prueba.Sala->CapacidadInventario, 2);
	return true;
}

#endif
