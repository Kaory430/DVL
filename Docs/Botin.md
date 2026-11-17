# Robo e inventario

## Reglas

- Solo los ladrones activos pueden robar. Mantener E durante **10 segundos**
  recoge el objeto mas cercano dentro de su AlcanceRobo (200 cm por defecto).
- Soltar E, salir del alcance, ser derribado, perder el controlador o que otro
  ladron recoja/devuelva ese objeto cancela el intento. Volver a intentarlo
  empieza desde cero. No se recoge nada automaticamente al pasar.
- Cada ladron puede llevar **3 objetos con 3 jugadores totales** y **2 objetos
  con 4 o 5 jugadores totales**, contando al policia. Se exige un policia y al
  menos dos ladrones para iniciar. No se duplica un mismo objeto.
  Dos ladrones pueden intentarlo a la vez; solo el primero que complete lo obtiene.
- E da prioridad a liberar una celda, despues a reanimar y finalmente a robar.
  No se pueden realizar dos de estas acciones a la vez.
- Al derribar al ladron caen todos sus objetos, tambien en su ultimo derribo.
  Se colocan alrededor del personaje sobre el suelo, sin simulacion fisica.
  Desconectarse/destruir el personaje tambien libera el inventario.
- Otro ladron puede robar un objeto caido manteniendo E otros 10 segundos.
- Si un policia toca la EsferaRecoleccion de un objeto caido, este regresa
  automaticamente a su posicion, rotacion y escala del inicio de la partida.
  No hace falta pulsar E. Si el policia ya esta encima al caer, se devuelve
  inmediatamente. Pasar por objetos que aun estan en su origen no los cambia.
- Si no hay suelo bajo la posicion de caida, se devuelve el objeto a su origen
  con aviso al jugador y en el registro, para no perderlo fuera del mapa.
- **Tab** abre/cierra el inventario; empieza cerrado. El HUD de ladrones muestra
  su mochila y los nombres/cantidades de objetos de
  sus companeros. El resumen de equipo se replica mediante PlayerState, incluso
  cuando el personaje del companero queda fuera de relevancia de red.
  El policia no muestra ese panel. Esto es visibilidad de interfaz, no un
  mecanismo para ocultar datos de red frente a clientes modificados.
- Inventario, temporizadores, capacidad y devoluciones los decide el servidor.
  Los ladrones ganan al tener los **5 artefactos simultaneamente entre sus
  mochilas** antes de los 15 minutos. No hay deposito ni entregas acumuladas.
  El mapa debe contener exactamente cinco actores derivados de Botin,
  incluidos los de prueba; retira los sobrantes. Consulta [MenuLAN.md](MenuLAN.md)
  para resultados, revancha y regreso a la sala.

## 1. Actualizar los Blueprints existentes

Guarda y cierra Unreal antes de compilar ProyectoDVLEditor. Abre otra vez al
terminar; hay propiedades C++ nuevas y no conviene actualizarlas con Live Coding.

### BP_Botin

Conserva BP_Botin, cuyo padre es Botin. Tus capturas muestran que es de solo datos:
no necesita Event Graph, temporizadores ni Destroy Actor.

En **Valores predeterminados de clase**, comprueba:

- **Replication > Replicates** y **Replicate Movement** activados.
- **Always Relevant** activado (son pocos objetos y deben estar sincronizados).
- En Mesh: **Mobility = Movable**, **Simulate Physics = desactivado**.
- En EsferaRecoleccion: **Generate Overlap Events = activado**, colision
  **Query Only**, respuesta **Pawn = Overlap**, las demas Ignore.
- **Sphere Radius = 55** como punto de partida para la devolucion policial.
  No es el alcance del robo: ese se ajusta en la propiedad **Botin > Alcance Robo**.
  El radio de la esfera se escala con el actor; revisalo si usas escalas grandes.

El C++ fuerza replicacion/movimiento y colision visual segura al comenzar, pero
conviene guardar estos valores tambien en el BP para no conservar ajustes viejos.

### BP_Ladron: sustituir el robo anterior

En el Event Graph de tu tercera captura:

1. Retira **la logica antigua de robo**: las conexiones a Botin En Rango,
   Set Timer by Event de 3 segundos, Clear and Invalidate Timer by Handle,
   RoboCompletado, el Set de Botin Recolectado y Destroy Actor del botin.
   El contador ahora es de solo lectura y lo mantiene el servidor.
2. Si tienes eventos de overlap que solo rellenan Botin En Rango para ese
   sistema antiguo, tambien puedes retirarlos. El C++ busca el objetivo.
3. Deja **un solo evento de la tecla E**:

   ```text
   E Pressed  -> Iniciar Interaccion   (Target = Self)
   E Released -> Cancelar Interaccion (Target = Self)
   ```

4. Si E tambien estaba conectada a Iniciar Reanimacion / Cancelar Reanimacion,
   sustituye esas conexiones por las anteriores. Las nuevas funciones incluyen
   robo, rescate y reanimacion: no llames a ambas parejas ni uses Sequence.
5. Si usas Enhanced Input en lugar de la tecla E directa, conecta Started a
   Iniciar Interaccion, y Completed y Canceled a Cancelar Interaccion.
   No uses Triggered para reiniciar una interaccion cada frame.
6. Compila y guarda BP_Ladron. La E de BP_Policia sigue como antes; no se cambia.

La barra de progreso, mensajes y mochila aparecen desde C++; no crees otro HUD.

## 2. Convertir tus objetos del mapa en robables

No basta con poner BP_Botin al lado de un objeto decorativo: el sistema solo
oculta/mueve su propio Mesh, no otros actores que tenga cerca.

Para cada modelo distinto (televisor, cuadro, joya, etc.):

1. En el Content Browser, duplica BP_Botin y nombra la copia, por ejemplo,
   **BP_Botin_Televisor**. Otra opcion es crear un Blueprint hijo de BP_Botin.
   Todos deben conservar Botin como clase C++ base.
2. Abre la copia. Si solo ves valores predeterminados, pulsa **Abrir el editor
   de blueprints completo**. En Componentes selecciona **Mesh** y asigna en
   **Static Mesh** el modelo que ya estas usando para ese objeto del escenario.
   Puedes localizar ese recurso seleccionando el objeto original en el mapa y
   usando la lupa junto a su Static Mesh.
3. En **Valores predeterminados de clase > Botin > Nombre Objeto**, escribe el
   nombre que quieres en el inventario, por ejemplo **Televisor**.
4. Compila/guarda y arrastra el nuevo BP al mapa. Copia posicion, rotacion y escala
   del objeto original, comprobando visualmente que coincidan.
5. Comprueba el modelo nuevo y solo entonces elimina **ese objeto decorativo
   original** del mapa, no el recurso Static Mesh del Content Browser.
   Si dejas los dos superpuestos, al robar seguira viendose el modelo decorativo.
6. Ajusta EsferaRecoleccion cerca del objeto y a una altura que pueda tocar la
   capsula del policia cuando el objeto este caido. No necesitas nodos de overlap.
7. Repite con los demas modelos y guarda el mapa. Puedes colocar varias instancias
   del mismo BP; cada una recuerda su propio origen. Nombre Objeto y Alcance Robo
   tambien se pueden cambiar individualmente desde Detalles del actor del mapa.

Si un objeto contiene varias mallas/actores independientes, primero selecciona
una malla principal o prepara un BP hijo con los componentes visuales unidos a
Mesh. El sistema oculta Mesh y sus hijos; no controla actores externos sueltos.

## Comprobacion manual (anfitrion y cliente)

1. Acercate a un botin: pasar no lo recoge; aparece el mensaje E con su nombre.
2. Manten E 9 segundos: aun no debe estar en la mochila. Completa los 10 segundos:
   desaparece del mundo y aparece una sola vez en ambos HUD de ladrones.
3. Prueba soltar E y alejarte a mitad: el progreso vuelve a cero.
4. Llena la mochila (3 plazas con 3 jugadores, 2 con 4 o 5):
   no debe poder recogerse otro objeto. Tab abre/cierra el panel sin parar el juego.
5. Intenta el mismo objeto con dos ladrones: no se duplica.
6. Derriba al portador: su mochila se vacia y caen los objetos para todos.
   Si el policia ya esta tocandolos, pueden volver inmediatamente al origen.
7. Con el policia lejos, roba un caido con otro ladron: vuelve a exigir 10 segundos.
8. Pasa con el policia por los caidos: regresan a su ubicacion original para todos.
9. Comprueba E cerca de una celda ocupada y de un companero derribado: rescate y
   reanimacion conservan su prioridad y duracion anterior.
10. Desconecta un ladron que lleva objetos: no deben desaparecer con el personaje.
