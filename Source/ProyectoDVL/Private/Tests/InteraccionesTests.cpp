#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Celda.h"
#include "Botin.h"
#include "ControladorJugadorDVL.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "HealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PersonajeLadron.h"
#include "PersonajePolicia.h"
#include "PowerupComponent.h"
#include "PuntoPowerup.h"

namespace
{
	struct FMundoInteracciones
	{
		UWorld* Mundo;

		FMundoInteracciones()
		{
			Mundo = UWorld::CreateWorld(EWorldType::Game, false);
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(Mundo);
			Mundo->InitializeActorsForPlay(FURL());

			AActor* Suelo = Mundo->SpawnActor<AActor>();
			UBoxComponent* Colision = NewObject<UBoxComponent>(Suelo);
			Suelo->SetRootComponent(Colision);
			Colision->SetBoxExtent(FVector(2000.0f, 2000.0f, 20.0f));
			Colision->SetCollisionProfileName(TEXT("BlockAll"));
			Colision->RegisterComponent();
			Suelo->SetActorLocation(FVector(0.0f, 0.0f, -20.0f));
			Mundo->GetWorldSettings()->NotifyBeginPlay();
		}

		~FMundoInteracciones()
		{
			Mundo->DestroyWorld(false);
			GEngine->DestroyWorldContext(Mundo);
		}

		template<typename T>
		T* CrearPersonaje(const FVector& Posicion)
		{
			FActorSpawnParameters Parametros;
			Parametros.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			T* Personaje = Mundo->SpawnActor<T>(Posicion, FRotator::ZeroRotator, Parametros);
			Personaje->SetActorTickEnabled(false);
			APlayerController* Controlador = Mundo->SpawnActor<APlayerController>();
			if (!Controlador->PlayerState)
			{
				Controlador->PlayerState = Mundo->SpawnActor<APlayerState>();
			}
			Controlador->Possess(Personaje);
			return Personaje;
		}

		void Avanzar(float Segundos)
		{
			const int32 Pasos = FMath::RoundToInt(Segundos * 10.0f);
			for (int32 Paso = 0; Paso < Pasos; ++Paso)
			{
				Mundo->Tick(LEVELTICK_All, 0.1f);
			}
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAdrenalinaDVLTest, "ProyectoDVL.Powerups.Adrenalina",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FAdrenalinaDVLTest::RunTest(const FString& Parameters)
{
	FMundoInteracciones Prueba;
	APersonajeLadron* Ladron = Prueba.CrearPersonaje<APersonajeLadron>(FVector(0.0f, 0.0f, 100.0f));
	UPowerupComponent* Powerup = Ladron->ComponentePowerup;
	Ladron->GetCharacterMovement()->MaxWalkSpeed = 450.0f;
	TestFalse(TEXT("El ladron no puede recoger pulso"), Powerup->Recoger(EPowerupDVL::Pulso));
	TestTrue(TEXT("Recoge una adrenalina"), Powerup->Recoger(EPowerupDVL::Adrenalina));
	TestFalse(TEXT("No guarda dos cargas"), Powerup->Recoger(EPowerupDVL::Adrenalina));
	Powerup->Activar();
	TestTrue(TEXT("Activar consume la carga"), Powerup->Carga == EPowerupDVL::Ninguno);
	TestTrue(TEXT("La velocidad sube exactamente treinta por ciento"), FMath::IsNearlyEqual(Ladron->GetCharacterMovement()->MaxWalkSpeed, 585.0f));
	Powerup->Activar();
	TestTrue(TEXT("Volver a pulsar Q no acumula velocidad"), FMath::IsNearlyEqual(Ladron->GetCharacterMovement()->MaxWalkSpeed, 585.0f));
	Prueba.Avanzar(5.5f);
	TestTrue(TEXT("La adrenalina sigue activa antes de seis segundos"), Powerup->Efecto == EPowerupDVL::Adrenalina);
	Prueba.Avanzar(0.8f);
	TestTrue(TEXT("Termina despues de seis segundos"), Powerup->Efecto == EPowerupDVL::Ninguno);
	TestEqual(TEXT("Recupera su velocidad anterior, no un valor fijo"), Ladron->GetCharacterMovement()->MaxWalkSpeed, 450.0f);
	Powerup->Recoger(EPowerupDVL::Adrenalina);
	Powerup->Activar();
	UGameplayStatics::ApplyDamage(Ladron, 100.0f, nullptr, nullptr, nullptr);
	TestTrue(TEXT("Ser derribado cancela el efecto inmediatamente"), Powerup->Efecto == EPowerupDVL::Ninguno);
	TestEqual(TEXT("Derribarlo restaura la velocidad"), Ladron->GetCharacterMovement()->MaxWalkSpeed, 450.0f);
	TestFalse(TEXT("Un derribado no recoge cargas"), Powerup->Recoger(EPowerupDVL::Adrenalina));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPulsoYRecogidaDVLTest, "ProyectoDVL.Powerups.PulsoYRecogida",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FPulsoYRecogidaDVLTest::RunTest(const FString& Parameters)
{
	FMundoInteracciones Prueba;
	APersonajePolicia* Policia = Prueba.CrearPersonaje<APersonajePolicia>(FVector(0.0f, 0.0f, 100.0f));
	APersonajeLadron* Ladron = Prueba.CrearPersonaje<APersonajeLadron>(FVector(1500.0f, 0.0f, 100.0f));
	UPowerupComponent* Powerup = Policia->ComponentePowerup;
	TestTrue(TEXT("El pulso incluye el limite de quince metros"), Powerup->PuedeDetectar(Ladron));
	Ladron->SetActorLocation(FVector(1501.0f, 0.0f, 100.0f));
	TestFalse(TEXT("No revela fuera del radio"), Powerup->PuedeDetectar(Ladron));
	Ladron->SetActorLocation(FVector(1000.0f, 0.0f, 100.0f));
	ACelda* Celda = Prueba.Mundo->SpawnActor<ACelda>();
	Ladron->CeldaActual = Celda;
	TestFalse(TEXT("No revela encarcelados"), Powerup->PuedeDetectar(Ladron));
	Ladron->CeldaActual = nullptr;
	AController* ControladorLadron = Ladron->GetController();
	ControladorLadron->UnPossess();
	TestFalse(TEXT("El servidor excluye personajes abandonados"), Powerup->PuedeDetectar(Ladron));
	Policia->SetRole(ROLE_AutonomousProxy);
	TestTrue(TEXT("El cliente revela objetivos remotos sin su PlayerController"), Powerup->PuedeDetectar(Ladron));
	Policia->SetRole(ROLE_Authority);
	ControladorLadron->Possess(Ladron);
	TestFalse(TEXT("El policia no recibe adrenalina"), Powerup->Recoger(EPowerupDVL::Adrenalina));
	APuntoPowerup* Punto = Prueba.Mundo->SpawnActor<APuntoPowerup>(FVector(500.0f, 500.0f, 0.0f), FRotator::ZeroRotator);
	Punto->SetActorLocation(FVector::ZeroVector);
	Prueba.Avanzar(0.2f);
	TestTrue(TEXT("Pasar por el punto entrega el pulso"), Powerup->Carga == EPowerupDVL::Pulso);
	TestFalse(TEXT("El punto queda consumido"), Punto->bDisponible);
	TestFalse(TEXT("No puede recogerse dos veces"), Punto->IntentarRecoger(Policia));
	Powerup->Activar();
	Prueba.Avanzar(4.5f);
	TestTrue(TEXT("Pulso activo antes de cinco segundos"), Powerup->Efecto == EPowerupDVL::Pulso);
	Prueba.Avanzar(0.8f);
	TestTrue(TEXT("El pulso termina a los cinco segundos"), Powerup->Efecto == EPowerupDVL::Ninguno);
	Policia->SetActorLocation(FVector(-1000.0f, 0.0f, 100.0f));
	Prueba.Avanzar(84.0f);
	TestFalse(TEXT("El punto no reaparece antes de noventa segundos"), Punto->bDisponible);
	Prueba.Avanzar(1.0f);
	TestTrue(TEXT("El punto vuelve a estar disponible a los noventa segundos"), Punto->bDisponible);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAvisosPartidaDVLTest, "ProyectoDVL.Partida.AvisosTemporales",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FAvisosPartidaDVLTest::RunTest(const FString& Parameters)
{
	FMundoInteracciones Prueba;
	AControladorJugadorDVL* Controlador = Prueba.Mundo->SpawnActor<AControladorJugadorDVL>();
	Controlador->SetActorTickEnabled(false);
	Controlador->Client_AvisoPartida_Implementation(FText::FromString(TEXT("Robaron Figura alienigena.")));
	Prueba.Avanzar(3.0f);
	Controlador->Client_AvisoPartida_Implementation(FText::FromString(TEXT("Derribaron a Jugador.")));
	Prueba.Avanzar(4.2f);
	const FString Avisos = Controlador->ObtenerAvisosPartida().ToString();
	TestFalse(TEXT("El primer aviso caduca de forma independiente"), Avisos.Contains(TEXT("Figura alienigena")));
	TestTrue(TEXT("El aviso mas reciente sigue visible"), Avisos.Contains(TEXT("Jugador")));
	Controlador->Client_AvisoPartida_Implementation(FText::FromString(TEXT("Devolvieron Televisor.")));
	Controlador->Client_AvisoPartida_Implementation(FText::FromString(TEXT("Devolvieron Cuadro.")));
	Controlador->Client_AvisoPartida_Implementation(FText::FromString(TEXT("Robaron Joya.")));
	Controlador->Client_AvisoPartida_Implementation(FText::FromString(TEXT("Robaron Estatua.")));
	const FString Rafaga = Controlador->ObtenerAvisosPartida().ToString();
	TestFalse(TEXT("Una rafaga retira el aviso mas antiguo"), Rafaga.Contains(TEXT("Jugador")));
	TestTrue(TEXT("Una rafaga conserva el ultimo aviso"), Rafaga.Contains(TEXT("Estatua")));
	TestTrue(TEXT("Los avisos se muestran en orden de llegada"),
		Rafaga.Find(TEXT("Televisor")) < Rafaga.Find(TEXT("Estatua")));
	Prueba.Avanzar(7.2f);
	TestTrue(TEXT("El panel queda vacio al caducar los avisos"), Controlador->ObtenerAvisosPartida().IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoboTiempoCapacidadTest, "ProyectoDVL.Interacciones.RoboTiempoCapacidad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FRoboTiempoCapacidadTest::RunTest(const FString& Parameters)
{
	FMundoInteracciones Prueba;
	APersonajeLadron* Ladron = Prueba.CrearPersonaje<APersonajeLadron>(FVector(0.0f, 0.0f, 100.0f));
	ABotin* Primero = Prueba.Mundo->SpawnActor<ABotin>(FVector(0.0f, 0.0f, 10.0f), FRotator::ZeroRotator);
	Ladron->IniciarInteraccion();
	Prueba.Avanzar(5.0f);
	TestTrue(TEXT("El robo progresa durante diez segundos"), FMath::IsNearlyEqual(Ladron->ProgresoRobo, 0.5f, 0.03f));
	TestTrue(TEXT("No recoge antes de tiempo"), Ladron->Inventario.IsEmpty());
	Ladron->CancelarInteraccion();
	TestFalse(TEXT("Soltar E cancela"), Ladron->bRobando);
	Ladron->IniciarInteraccion();
	Ladron->SetActorLocation(FVector(600.0f, 0.0f, 100.0f));
	Prueba.Avanzar(0.2f);
	TestFalse(TEXT("Alejarse tambien cancela"), Ladron->bRobando);
	Ladron->SetActorLocation(FVector(0.0f, 0.0f, 100.0f));
	Ladron->IniciarInteraccion();
	Prueba.Avanzar(9.8f);
	TestTrue(TEXT("Reiniciar no conserva el progreso anterior"), Ladron->Inventario.IsEmpty());
	Prueba.Avanzar(0.4f);
	TestEqual(TEXT("Completar recoge exactamente un objeto"), Ladron->Inventario.Num(), 1);
	TestTrue(TEXT("El objeto se conserva como transportado"), Primero->Estado == EEstadoBotin::Transportado);
	for (int32 Indice = 0; Indice < 3; ++Indice)
	{
		Prueba.Mundo->SpawnActor<ABotin>(FVector(0.0f, 0.0f, 10.0f), FRotator::ZeroRotator);
		Ladron->IniciarInteraccion();
		Prueba.Avanzar(10.2f);
	}
	TestEqual(TEXT("No permite recoger un cuarto objeto"), Ladron->Inventario.Num(), 3);
	TestFalse(TEXT("Una mochila llena no inicia robo"), Ladron->bRobando);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBotinCaidaDevolucionTest, "ProyectoDVL.Interacciones.BotinCaidaDevolucion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FBotinCaidaDevolucionTest::RunTest(const FString& Parameters)
{
	FMundoInteracciones Prueba;
	APersonajeLadron* Primero = Prueba.CrearPersonaje<APersonajeLadron>(FVector(0.0f, 0.0f, 100.0f));
	APersonajeLadron* Segundo = Prueba.CrearPersonaje<APersonajeLadron>(FVector(0.0f, 120.0f, 100.0f));
	APersonajePolicia* Policia = Prueba.CrearPersonaje<APersonajePolicia>(FVector(1000.0f, 0.0f, 100.0f));
	const FTransform Origen(FRotator(0.0f, 45.0f, 0.0f), FVector(0.0f, 0.0f, 10.0f), FVector(1.0f));
	ABotin* Botin = Prueba.Mundo->SpawnActor<ABotin>(ABotin::StaticClass(), Origen);
	Primero->IniciarInteraccion();
	Segundo->IniciarInteraccion();
	Prueba.Avanzar(10.2f);
	TestEqual(TEXT("Dos intentos no duplican el objeto"), Primero->Inventario.Num() + Segundo->Inventario.Num(), 1);
	APersonajeLadron* Portador = Primero->Inventario.Num() == 1 ? Primero : Segundo;
	APersonajeLadron* Libre = Portador == Primero ? Segundo : Primero;
	UGameplayStatics::ApplyDamage(Portador, 100.0f, Policia->GetController(), Policia, nullptr);
	TestTrue(TEXT("El derribo vacia la mochila"), Portador->Inventario.IsEmpty());
	TestTrue(TEXT("El objeto cae y no se destruye"), Botin->Estado == EEstadoBotin::Caido);
	const FVector Direccion = (Botin->GetActorLocation() - Portador->GetActorLocation()).GetSafeNormal2D();
	Libre->SetActorLocation(Botin->GetActorLocation() + Direccion * 170.0f + FVector(0.0f, 0.0f, 90.0f));
	Libre->IniciarInteraccion();
	Prueba.Avanzar(10.2f);
	TestEqual(TEXT("Otro ladron puede robar el caido"), Libre->Inventario.Num(), 1);
	UGameplayStatics::ApplyDamage(Libre, 100.0f, Policia->GetController(), Policia, nullptr);
	Policia->SetActorLocation(Botin->GetActorLocation() + FVector(0.0f, 0.0f, 60.0f));
	Prueba.Avanzar(0.2f);
	TestTrue(TEXT("El policia devuelve automaticamente el caido"), Botin->Estado == EEstadoBotin::EnOrigen);
	TestTrue(TEXT("Se restaura la transformacion original"), Botin->GetActorTransform().Equals(Origen));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReanimacionYCargaTest, "ProyectoDVL.Interacciones.ReanimacionYCarga",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FReanimacionYCargaTest::RunTest(const FString& Parameters)
{
	FMundoInteracciones Prueba;
	APersonajeLadron* Objetivo = Prueba.CrearPersonaje<APersonajeLadron>(FVector(0.0f, 0.0f, 100.0f));
	APersonajeLadron* Ayudante = Prueba.CrearPersonaje<APersonajeLadron>(FVector(0.0f, 130.0f, 100.0f));
	APersonajePolicia* Policia = Prueba.CrearPersonaje<APersonajePolicia>(FVector(130.0f, 0.0f, 100.0f));
	UGameplayStatics::ApplyDamage(Objetivo, 100.0f, Policia->GetController(), Policia, nullptr);
	Ayudante->IniciarReanimacion();
	Prueba.Avanzar(5.0f);
	TestTrue(TEXT("A los cinco segundos el progreso es aproximadamente 50%"), FMath::IsNearlyEqual(Ayudante->ProgresoReanimacion, 0.5f, 0.03f));
	TestTrue(TEXT("No se levanta antes de diez segundos"), Objetivo->bDerribado);
	TestEqual(TEXT("No consume vidas antes de completar"), Objetivo->VidasRestantes, 3);

	Policia->CargarOSoltarLadron();
	TestEqual(TEXT("El policia lleva al objetivo"), Policia->LadronCargado, Objetivo);
	TestTrue(TEXT("El objetivo queda marcado como cargado"), Objetivo->bSiendoCargado);
	TestFalse(TEXT("Transportarlo cancela inmediatamente la reanimacion"), Ayudante->bReanimando);
	TestEqual(TEXT("La cancelacion reinicia la barra"), Ayudante->ProgresoReanimacion, 0.0f);
	TestFalse(TEXT("El transportado no bloquea al policia"), Objetivo->GetActorEnableCollision());
	TestEqual(TEXT("Se adjunta al portador"), Objetivo->GetAttachParentActor(), static_cast<AActor*>(Policia));
	Ayudante->IniciarReanimacion();
	TestFalse(TEXT("No se puede reanimar al transportado"), Ayudante->bReanimando);

	Policia->CargarOSoltarLadron();
	TestNull(TEXT("El policia deja de cargar"), Policia->LadronCargado);
	TestFalse(TEXT("Se elimina el estado cargado"), Objetivo->bSiendoCargado);
	TestNull(TEXT("El objetivo queda separado"), Objetivo->GetAttachParentActor());
	TestTrue(TEXT("Se restaura la colision"), Objetivo->GetActorEnableCollision());
	TestEqual(TEXT("Soltar no reanima ni permite caminar"), Objetivo->GetCharacterMovement()->MovementMode, MOVE_None);
	Ayudante->SetActorLocation(Objetivo->GetActorLocation() + FVector(0.0f, 130.0f, 0.0f));
	Ayudante->IniciarReanimacion();
	Prueba.Avanzar(10.2f);
	TestFalse(TEXT("Puede reanimarse tras soltarlo"), Objetivo->bDerribado);
	TestEqual(TEXT("Recupera toda su salud"), Objetivo->ComponenteSalud->SaludActual, Objetivo->ComponenteSalud->SaludMaxima);
	TestEqual(TEXT("Solo consume una vida"), Objetivo->VidasRestantes, 2);
	TestEqual(TEXT("Recupera el movimiento"), Objetivo->GetCharacterMovement()->MovementMode, MOVE_Walking);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLimitesCargaTest, "ProyectoDVL.Interacciones.LimitesCarga",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FLimitesCargaTest::RunTest(const FString& Parameters)
{
	FMundoInteracciones Prueba;
	APersonajeLadron* Objetivo = Prueba.CrearPersonaje<APersonajeLadron>(FVector(0.0f, 0.0f, 100.0f));
	APersonajePolicia* Policia = Prueba.CrearPersonaje<APersonajePolicia>(FVector(300.0f, 0.0f, 100.0f));
	APersonajePolicia* OtroPolicia = Prueba.CrearPersonaje<APersonajePolicia>(FVector(-130.0f, 0.0f, 100.0f));
	TestNull(TEXT("No se puede recoger un ladron activo"), OtroPolicia->BuscarLadronParaCargar());
	UGameplayStatics::ApplyDamage(Objetivo, 100.0f, Policia->GetController(), Policia, nullptr);
	TestNull(TEXT("No se puede recoger fuera de alcance"), Policia->BuscarLadronParaCargar());
	Policia->SetActorLocation(FVector(130.0f, 0.0f, 100.0f));
	Policia->CargarOSoltarLadron();
	TestNull(TEXT("Dos policias no pueden cargar al mismo ladron"), OtroPolicia->BuscarLadronParaCargar());

	Policia->Destroy();
	TestFalse(TEXT("Destruir al portador libera al prisionero"), Objetivo->bSiendoCargado);
	TestNull(TEXT("No queda un adjunto a un actor destruido"), Objetivo->GetAttachParentActor());
	TestTrue(TEXT("Se restaura la colision al desaparecer el portador"), Objetivo->GetActorEnableCollision());
	TestTrue(TEXT("La liberacion no consume vidas ni reanima"), Objetivo->bDerribado && Objetivo->VidasRestantes == 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCeldaRescateTest, "ProyectoDVL.Interacciones.CeldaRescate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FCeldaRescateTest::RunTest(const FString& Parameters)
{
	FMundoInteracciones Prueba;
	ACelda* Celda = Prueba.Mundo->SpawnActor<ACelda>();
	APersonajeLadron* Objetivo = Prueba.CrearPersonaje<APersonajeLadron>(FVector(0.0f, 0.0f, 100.0f));
	APersonajeLadron* Ayudante = Prueba.CrearPersonaje<APersonajeLadron>(FVector(0.0f, 130.0f, 100.0f));
	APersonajePolicia* Policia = Prueba.CrearPersonaje<APersonajePolicia>(FVector(130.0f, 0.0f, 100.0f));
	// El ultimo encarcelamiento puede consumir la ultima vida sin eliminar al jugador.
	Objetivo->VidasRestantes = 1;
	UGameplayStatics::ApplyDamage(Objetivo, 100.0f, Policia->GetController(), Policia, nullptr);
	Policia->CargarOSoltarLadron();
	Policia->CargarOSoltarLadron();
	TestEqual(TEXT("La celda tiene un ocupante"), Celda->Prisionero, Objetivo);
	TestEqual(TEXT("El ladron conoce su celda"), Objetivo->CeldaActual, Celda);
	TestEqual(TEXT("Encarcelar consume exactamente una vida"), Objetivo->VidasRestantes, 0);
	TestNull(TEXT("El policia deja de transportarlo"), Policia->LadronCargado);
	TestFalse(TEXT("El prisionero ya no esta cargado"), Objetivo->bSiendoCargado);
	TestEqual(TEXT("El encarcelado no puede caminar"), Objetivo->GetCharacterMovement()->MovementMode, MOVE_None);
	TestNull(TEXT("El preso no es candidato a reanimacion normal"), Ayudante->BuscarCompaneroReanimable());
	TestNull(TEXT("El policia no puede recoger al encarcelado"), Policia->BuscarLadronParaCargar());

	Ayudante->SetActorLocation(FVector(200.0f, 90.0f, 100.0f));
	Ayudante->IniciarReanimacion();
	Prueba.Avanzar(5.0f);
	TestTrue(TEXT("E en la zona inicia rescate con progreso"), Ayudante->bRescatando && Ayudante->ProgresoReanimacion > 0.45f);
	Ayudante->CancelarReanimacion();
	TestEqual(TEXT("Cancelar no libera al prisionero"), Celda->Prisionero, Objetivo);
	TestEqual(TEXT("Cancelar no descuenta otra vida"), Objetivo->VidasRestantes, 0);
	Ayudante->IniciarReanimacion();
	Prueba.Avanzar(9.8f);
	TestEqual(TEXT("No libera antes de diez segundos nuevos"), Celda->Prisionero, Objetivo);
	Prueba.Avanzar(0.4f);
	TestNull(TEXT("La celda queda libre"), Celda->Prisionero);
	TestNull(TEXT("El liberado ya no esta encarcelado"), Objetivo->CeldaActual);
	TestFalse(TEXT("Liberar elimina el derribo"), Objetivo->bDerribado);
	TestEqual(TEXT("Sale con salud completa"), Objetivo->ComponenteSalud->SaludActual, Objetivo->ComponenteSalud->SaludMaxima);
	TestEqual(TEXT("Liberar no cobra otra vida ni baja de cero"), Objetivo->VidasRestantes, 0);
	TestEqual(TEXT("Recupera movimiento con cero vidas"), Objetivo->GetCharacterMovement()->MovementMode, MOVE_Walking);
	UGameplayStatics::ApplyDamage(Objetivo, 100.0f, Policia->GetController(), Policia, nullptr);
	TestTrue(TEXT("Con cero vidas muere en el siguiente derribo"), Objetivo->IsActorBeingDestroyed());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCeldaOcupacionTest, "ProyectoDVL.Interacciones.CeldaOcupacion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FCeldaOcupacionTest::RunTest(const FString& Parameters)
{
	FMundoInteracciones Prueba;
	ACelda* Celda = Prueba.Mundo->SpawnActor<ACelda>();
	APersonajeLadron* Objetivo = Prueba.CrearPersonaje<APersonajeLadron>(FVector(0.0f, 0.0f, 100.0f));
	APersonajePolicia* Policia = Prueba.CrearPersonaje<APersonajePolicia>(FVector(130.0f, 0.0f, 100.0f));
	UGameplayStatics::ApplyDamage(Objetivo, 100.0f, Policia->GetController(), Policia, nullptr);
	Policia->CargarOSoltarLadron();

	AActor* Obstaculo = Prueba.Mundo->SpawnActor<AActor>();
	UBoxComponent* Caja = NewObject<UBoxComponent>(Obstaculo);
	Obstaculo->SetRootComponent(Caja);
	Caja->SetBoxExtent(FVector(40.0f, 40.0f, 90.0f));
	Caja->SetCollisionProfileName(TEXT("BlockAll"));
	Caja->RegisterComponent();
	Obstaculo->SetActorLocation(FVector(0.0f, 0.0f, 100.0f));
	FString Error;
	TestFalse(TEXT("El interior bloqueado impide encarcelar"), Celda->Encarcelar(Policia, Error));
	TestFalse(TEXT("La colocacion invalida explica el error"), Error.IsEmpty());
	TestEqual(TEXT("El intento fallido no consume vida"), Objetivo->VidasRestantes, 3);
	TestEqual(TEXT("El intento fallido no suelta al transportado"), Policia->LadronCargado, Objetivo);
	Obstaculo->Destroy();
	Policia->CargarOSoltarLadron();
	TestEqual(TEXT("Al despejar el interior puede encarcelarse"), Celda->Prisionero, Objetivo);

	APersonajeLadron* Segundo = Prueba.CrearPersonaje<APersonajeLadron>(FVector(500.0f, 0.0f, 100.0f));
	APersonajePolicia* OtroPolicia = Prueba.CrearPersonaje<APersonajePolicia>(FVector(630.0f, 0.0f, 100.0f));
	UGameplayStatics::ApplyDamage(Segundo, 100.0f, OtroPolicia->GetController(), OtroPolicia, nullptr);
	OtroPolicia->CargarOSoltarLadron();
	OtroPolicia->SetActorLocation(FVector(200.0f, -90.0f, 100.0f));
	TestFalse(TEXT("La celda ocupada rechaza a un segundo prisionero"), Celda->Encarcelar(OtroPolicia, Error));
	TestEqual(TEXT("El segundo prisionero no pierde vida"), Segundo->VidasRestantes, 3);
	TestEqual(TEXT("El primer ocupante se conserva"), Celda->Prisionero, Objetivo);
	Objetivo->Destroy();
	TestNull(TEXT("Al desconectarse o destruirse el preso queda disponible"), Celda->Prisionero);
	OtroPolicia->CargarOSoltarLadron();
	TestEqual(TEXT("Se puede reutilizar la celda vacia"), Celda->Prisionero, Segundo);
	return true;
}

#endif
