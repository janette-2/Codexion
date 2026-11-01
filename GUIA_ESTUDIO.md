# Guía de estudio de Codexion

> Objetivo: no memorizar los archivos de arriba abajo, sino poder reconstruir el
> flujo, justificar cada decisión y controlar cada fase sin mirar el código.

---

## 1. Modelo mental mínimo

Antes de entrar en detalles, memoriza esto:

- **Actores:** hilo principal, N hilos coder y 1 hilo monitor.
- **Cerrojos:** `mtx_dongles` protege el estado compartido; `mtx_log` protege
  stdout; `cond_dongles` es la sala de espera, no un lock.
- **Estados:** `SIM_RUNNING`, `SIM_STOPPED` (burnout) y `SIM_FINISHED` (éxito).
- **Dongle:** `FREE` o `BLOCKED`, más `available_at` para el cooldown.
- **Scheduler:** FIFO ordena por ticket; EDF por
  `last_compilation + time_to_burnout`, con ticket como desempate.
- **Finales:** éxito si todos completan sus compilaciones; burnout si el monitor
  detecta retraso; error si falla una inicialización o un hilo.

Regla central: **cada dato compartido tiene un único lock; el trabajo largo
ocurre sin locks.**

---

## 2. Orden de lectura

| Sesión | Leer | Objetivo |
|---|---|---|
| 1 | `REPASO.md`, `DIAGRAMAS.md`, mapa visual de `CONTROL.md` | Tener el dibujo general y el vocabulario |
| 2 | `codexion.h`, `main.c`, `init.c`, `sync.c`, `coders.c`, `dongles.c` | Entender arranque, estructuras, creación de hilos y limpieza |
| 3 | `compiling.c`, `resources.c` | Seguir el ciclo feliz de un coder |
| 4 | `queue.c`, `queue_aux.c`, `heap_tools.c`, `heap_tools_aux.c` | Entender elección FIFO/EDF y el heap |
| 5 | `simulation.c` | Entender monitor, éxito, burnout y precisión |
| 6 | Rutas de error en `main.c`, `coders.c`, `queue_aux.c`, `init.c` | Entender fallos parciales y cleanup |
| 7 | Todo desde memoria | Reconstruir pseudocódigo, dibujar y defender |

No leas `CONTROL.md` entero: úsalo como registro de decisiones. Para estudiar,
usa el mapa visual y los hallazgos.

---

## 3. Cómo leer cada función

Por cada función, haz una ficha de 5 líneas:

1. **Propósito:** qué problema resuelve.
2. **Quién la llama:** hilo principal, coder o monitor.
3. **Precondición:** qué debe ser cierto al entrar.
4. **Postcondición:** qué queda garantizado al salir.
5. **Lock:** qué lock tiene tomado y qué datos toca.

Ejemplo mental para `block_dongles`: entra un coder sin dongles, registra su
petición, espera, y solo sale con el par marcado `BLOCKED` y `last_compilation`
actualizado. Todo bajo `mtx_dongles`.

---

## 4. Los tres recorridos que debes poder dibujar

### Recorrido A — camino feliz de un coder

`main` → `run_simulation` → `start_coders` → `coder_routine` →
`compiling_phase` → `block_dongles` → `wait_for_dongles` → compila → libera →
debug → refactor → repite.

Preguntas de control:

- ¿Dónde se registra el ticket?
- ¿Dónde se actualiza `last_compilation`?
- ¿Por qué la compilación ocurre sin `mtx_dongles`?
- ¿Dónde se pone el cooldown?
- ¿Por qué `number_of_compiles_done` sube al final y no al principio?

### Recorrido B — elección FIFO/EDF

`register_request` → `enqueue_coder` → `heap_push` → `wait_for_dongles` →
`choose_next_in_queue` → comparador → `heap_pop` → `dequeue_coder`.

Preguntas de control:

- ¿Qué clave usa FIFO y cuál EDF?
- ¿Por qué el heap no puede ordenar solo por ticket/deadline?
- ¿Para qué existe el buffer `deferred`?
- ¿Por qué la elección se recalcula al despertar y no hay cursor?
- ¿Qué pasa si el coder elegido no puede usar su par?

### Recorrido C — terminación

Monitor → `check_simulation_state` → `scan_coders_locked` → éxito o burnout →
cambiar estado → `broadcast` → coders despiertan y salen → `join` de coders →
`join` del monitor → `destroy_sync` → `free_control`.

Preguntas de control:

- ¿Quién declara el burnout y por qué no el coder?
- ¿Por qué el monitor mira `number_of_compiles_done` antes que el burnout?
- ¿Por qué el monitor muestrea cada 5 ms?
- ¿Qué pasa con N=1?
- ¿Qué se destruye y en qué orden?

---

## 5. Los invariantes que debes poder recitar

1. Un coder nunca espera con un dongle en la mano.
2. El par de dongles se adquiere atómicamente bajo `mtx_dongles`.
3. Todo cambio de estado compartido ocurre bajo `mtx_dongles` y se avisa con
   `broadcast` antes de soltar.
4. Todo el trabajo largo (`usleep`, fases) ocurre sin locks.
5. El heap contiene solo peticiones registradas; `heap_pos` siempre apunta a la
   posición real.
6. El monitor es el único que declara burnout; el coder solo reacciona a
   `SIM_RUNNING`.

Si puedes explicar estos seis sin mirar, ya controlas la arquitectura.

---

## 6. Ejercicios de estudio

- Dibuja de memoria el ciclo de un coder y compáralo con `DIAGRAMAS.md`.
- Traza N=3 FIFO en papel: qué tickets salen y por qué.
- Traza N=4 EDF con dos deadlines distintos: quién gana y por qué.
- Explica qué pasa si falla `pthread_create` del coder 3.
- Explica qué pasa si falla `pthread_create` del monitor.
- Explica por qué `n == 1` termina por burnout.
- Cierra los archivos y escribe el pseudocódigo de `block_dongles`,
  `wait_for_dongles`, `choose_next_in_queue` y `check_simulation_state`.
- Compara tu pseudocódigo con el real y anota solo las diferencias.

---

## 7. Pruebas para observar cada capa

- Ciclo normal: `stdbuf -oL ./codexion 4 1000 10 10 10 2 30 fifo`.
- Burnout: `./codexion 2 15 100 10 10 1 1 fifo`.
- EDF viable: `./codexion 5 600 100 50 50 3 20 edf`.
- N=1: `./codexion 1 200 50 50 50 1 10 edf`.
- Dormido sin CPU: `ps -o %cpu,stat,wchan -C codexion` mientras corre.
- Tickets FIFO: usa `gdb` y un breakpoint en `choose_next_in_queue`.

---

## 8. Prueba final de dominio

Deberías poder responder en menos de un minuto:

- ¿Dónde está el arbitraje EDF?
- ¿Por qué no hay deadlock?
- ¿Por qué se usa heap?
- ¿Por qué el monitor cumple los 10 ms?
- ¿Qué pasa si el coder elegido no puede coger sus dongles?
- ¿Qué se libera si falla `malloc` de dongles?
- ¿Por qué `free(NULL)` ya no aparece?

---

## 9. Checklist de sesión

Antes de cerrar una sesión, comprueba que puedes:

- [ ] Explicar la fase sin mirar el código.
- [ ] Nombrar el lock que protege cada dato compartido.
- [ ] Dibujar el flujo o la línea temporal.
- [ ] Explicar el camino de error de esa fase.
- [ ] Escribir el pseudocódigo de memoria.
- [ ] Hacer una pregunta de defensa y responderla en una frase.
