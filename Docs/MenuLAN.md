# Menu y sala LAN

## Activacion inicial

1. Compila `ProyectoDVLEditor` con Unreal cerrado y abre el proyecto.
2. Abre `MenuMap`. En **World Settings > GameMode Override**, selecciona
   **ModoMenuDVL** y guarda el mapa. No cambies el GameMode de ThirdPersonMap:
   el viaje desde la sala selecciona el modo de juego correcto.
3. Quita la creacion automatica del menu anterior de su Level Blueprint o
   GameMode anterior, si la habia. El controlador nuevo crea la interfaz.
4. Comprueba en **Project Settings > Maps & Modes** que **Game Instance Class**
   sea **InstanciaJuegoDVL** y que **Game Default Map** sea MenuMap.
5. Inicia en **Standalone Game** con un solo jugador local. No uses las opciones
   de PIE que conectan automaticamente como cliente o listen server: omiten el
   flujo de crear y buscar sesiones.

Ya hay interfaces nativas funcionales. No necesitas crear widgets ni nodos para
probarlas. Crear una sala reutiliza MenuMap con ModoSalaDVL; comenzar viaja a
ThirdPersonMap con ModoJuegoDVL.

## Interfaz nativa Chase By Pigs

El menu generado en C++ tiene portada **Chase By Pigs**, menu principal y
pantallas separadas para crear y buscar salas. Entrar a Buscar inicia la busqueda;
**Actualizar salas** la repite. **Volver** regresa al menu sin crear una sesion.
Los errores y el estado de conexion permanecen visibles debajo de cada pantalla.
Durante una operacion de red se bloquea la navegacion para evitar solicitudes dobles.

La sala mantiene el mismo estilo oscuro, con azul para policias y ambar para
ladrones, tarjetas de jugadores, plazas vacias y estado listo individual.
El diseno se escala proporcionalmente a la ventana; no requiere imagenes ni
recursos adicionales. Para verlo, compila con Unreal cerrado y abre de nuevo.

Este aspecto se aplica cuando MenuDVL/SalaEsperaDVL generan su propia interfaz.
Los Widget Blueprints que ya tengan un RootWidget conservan su diseno y los
nombres de controles indicados abajo: no se reemplaza su arbol ni sus eventos.

## Viaje desde PIE

El proyecto habilita `net.AllowPIESeamlessTravel=1` en DefaultEngine.ini para
conservar conexiones y bandos al iniciar desde el editor. Reinicia Unreal si
acabas de actualizar esta configuracion. Si se desactiva durante la sesion, el
boton Iniciar muestra un aviso y no cierra la sala ni realiza un viaje destructivo.

Sin esta opcion, Unreal sustituye el viaje continuo por una reconexion completa:
se pierden los bandos del PlayerState y los clientes existentes son rechazados
por la regla que impide nuevas entradas con la partida iniciada.

Crear una sala sigue siendo distinto de iniciarla: crear usa OpenLevel y cierra
las conexiones que PIE haya establecido automaticamente antes de entrar al menu.
Por eso se recomienda Standalone y que cada cliente use Buscar/Unirse.

## Reglas

- Sesiones LAN de hasta cinco jugadores, incluyendo al anfitrion.
- Un policia y hasta cuatro ladrones. Los cupos se validan en el servidor.
- Cambiar de bando elimina el estado listo.
- Solo el anfitrion inicia, con todos listos, un policia y al menos dos ladrones
  (minimo tres jugadores totales).
- Se cierra la sala antes del viaje: no se aceptan nuevas conexiones en partida.
- Los bandos se conservan mediante seamless travel y se crean BP_Policia o
  BP_Ladron segun la seleccion.
- Abandonar la sala libera el cupo. Si alguien abandona durante una ronda o
  mientras se vota revancha, los restantes vuelven a la sala.
- Si el anfitrion abandona/cierra la sesion, los demas regresan al menu cuando
  Unreal detecta la perdida de conexion; no hay migracion de host.

## Rondas, resultado y revancha

- El mapa de juego debe tener **exactamente cinco** actores Botin (tambien
  cuentan las copias de prueba). Si no coincide, se muestra un error de
  configuracion y el anfitrion puede regresar a la sala.
- Se espera a que lleguen los personajes antes de iniciar el reloj del servidor
  de **15:00**. No se permiten interacciones mientras se prepara la ronda.
- **Tab** abre/cierra el inventario de ladrones. No necesita nodos en Blueprint.
- **Q** activa el powerup recogido: pulso del policia o adrenalina de ladrones.
  Los puntos se colocan siguiendo [Powerups](Powerups.md); no cuentan como botin.
- La mochila admite **3** objetos con **3 jugadores totales**, y **2** con **4 o 5**.
  La capacidad se fija al empezar la ronda; el total incluye al policia.
- Los ladrones ganan al reunir simultaneamente los cinco objetos en sus mochilas.
  Los objetos caidos o devueltos no cuentan. No se acumulan robos historicos.
- El policia gana al agotarse los 900 segundos o cuando todos los ladrones han
  sido eliminados definitivamente. Derribo y encarcelamiento no son eliminacion.
  En el instante exacto del limite ya no se admite una recogida tardia.
- El resultado muestra **Victoria/Derrota** segun el bando del jugador, tambien
  para los espectadores eliminados. Se bloquean movimiento e interacciones.
- Cada conectado, incluidos los eliminados, puede votar revancha. Solo la
  unanimidad recarga el mapa con los mismos bandos y reinicia personajes,
  salud, vidas, objetos, mochila, reloj y votos. No hay temporizador de votacion:
  se espera una respuesta o la decision del anfitrion.
- Un rechazo devuelve a todos a la sala sin expulsar a quien rechazo.
- **P** abre/cierra las opciones durante la ronda, tambien para espectadores.
  No pausa el servidor. El anfitrion dispone de **Terminar y volver a la sala**;
  el resto puede continuar o abandonar la sesion.
- Al regresar a la sala se borran **bandos y Listo**: todos deben elegir otra vez.
  La sesion LAN se termina y vuelve a anunciarse sin desconectar al grupo.
  Se pueden incorporar nuevos jugadores y volver a iniciar.
- **Abandonar sesion** es distinto de volver juntos a la sala: el saliente
  vuelve al menu; si era el anfitrion, el grupo pierde la conexion.

No agregues eventos Tab/Q/P en los Blueprints: el controlador C++ ya los enlaza.
Compila con Unreal cerrado y vuelve a abrirlo para cargar los cambios.

## Avisos de partida

El HUD muestra avisos temporales a la izquierda, sin abrir el inventario.
No necesitan widgets ni nodos adicionales. Llegan desde el servidor mediante
el controlador de cada destinatario, tambien a los espectadores de su bando.

- Al completar un robo, el policia recibe: **Los ladrones robaron [objeto].
  Les faltan [cantidad] artefactos por reunir.** Los ladrones reciben la
  version **Tu equipo robo...**.
- La cantidad se calcula de las mochilas reales despues de recoger el objeto.
  Es **5 menos los objetos que llevan en ese instante**, no un total historico:
  los caidos y devueltos vuelven a faltar.
- Al derribar a un ladron, su equipo ve **El policia ha derribado a [jugador]**.
  Se usa el nombre de PlayerState, incluso en el ultimo derribo. Si el dano
  procede de otra causa, se muestra **[jugador] ha sido derribado**, sin atribuir
  falsamente el golpe al policia.
- Al devolver un caido, ambos bandos ven **El policia devolvio [objeto] a su lugar**.
  Solo se anuncia una vez por devolucion real; no al pasar por un objeto en origen
  ni cuando el sistema devuelve uno por no encontrar suelo.

Se conservan hasta cuatro avisos recientes, siete segundos cada uno. Una rafaga
desplaza los mas antiguos. No bloquean controles y no usan Print String, por
lo que forman parte de la interfaz normal del juego. Resultados/opciones tienen
prioridad visual; los avisos caducan tambien mientras estan ocultos. Al cambiar
de mapa se limpian, sin trasladar mensajes de la ronda anterior.

Comprobacion manual: roba desde un cliente y comprueba el nombre y la cantidad en
el policia; derriba al portador y revisa el aviso en todos los ladrones; pasa por
el botin caido y comprueba la devolucion en ambos bandos sin avisos duplicados.

## Usar tu diseno sin nodos de logica

Puedes conservar una copia de WBP_MenuBase y cambiar su **Parent Class** a
**MenuDVL**. El diseno debe incluir estos controles, marcados **Is Variable**,
con nombres exactos:

| Nombre | Tipo |
|---|---|
| NombreSala | Editable Text Box |
| BotonCrear | Button |
| BotonBuscar | Button |
| BotonSalir | Button |
| ListaPartidas | Scroll Box |
| TextoEstado | Text Block |

El Scroll Box recibe automaticamente las filas con el boton para unirse.
No conectes OnClicked manualmente: C++ conecta los botones. Retira los eventos
anteriores que creaban sesiones, viajaban o cerraban el juego para evitar
operaciones duplicadas. Puedes cambiar fondos, tipografia, colores y distribucion.

Para la sala, crea un Widget Blueprint con **SalaEsperaDVL** como padre:

| Nombre | Tipo |
|---|---|
| BotonPolicia | Button |
| BotonLadron | Button |
| BotonListo | Button |
| BotonIniciar | Button |
| BotonVolver | Button |
| TextoJugadores | Text Block |
| TextoEstado | Text Block |

Usa un texto como "Listo / No listo" en BotonListo. TextoJugadores indica el estado
actual. BotonIniciar solo se muestra al anfitrion.

Crea un Blueprint de **InstanciaJuegoDVL** y asigna en sus valores predeterminados
**ClaseMenu** y **ClaseSala** a tus widgets. Selecciona ese Blueprint en
**Project Settings > Maps & Modes > Game Instance Class**.
No necesita Event Graph. Tambien permite asignar mapas, clases de personaje y
ContextoJuego. Los valores nativos apuntan a los mapas y personajes existentes.

## Prueba LAN

1. Ejecuta al menos tres instancias independientes, preferiblemente en equipos de la
   misma red privada. Permite Unreal/juego en el firewall para redes privadas.
2. En la primera, escribe un nombre y pulsa **Crear partida**.
3. En las otras dos, pulsa **Buscar partidas LAN** y **Unirse** en una fila.
4. Comprueba que solo uno puede ser policia, que todos pueden marcarse listos y
   que solo el anfitrion puede iniciar.
5. Coloca suficientes PlayerStart libres en ThirdPersonMap para los cinco jugadores.
   Comprueba movimiento, camara, interacciones y HUD con ambos personajes.
6. Prueba que otro cliente no puede entrar cuando la partida ya comenzo.
7. Prueba abandonar como cliente y cerrar el anfitrion.

LAN no busca por internet ni atraviesa routers/NAT. Steam y EOS no estan integrados.
Comprueba tambien victoria al reunir cinco, limite de tiempo, eliminacion,
revancha unanime, rechazo, salida de un cliente, regreso del anfitrion a la sala
y una segunda partida desde esa misma sala. Repite con un eliminado votando
revancha: debe volver a jugar con salud y vidas nuevas.

Las pruebas automatizadas de ProyectoDVL.Sala y ProyectoDVL.Partida verifican
reglas, copia de bandos, tiempo y condiciones de victoria;
no sustituyen esta prueba de conectividad y viaje entre equipos.

Al empaquetar, ya se incluyen los dos mapas y los directorios de Blueprints e Input
usados por defecto. Si los cambias por otros mapas o recursos referenciados solo
desde configuracion/C++, incluyelos tambien en Packaging.
