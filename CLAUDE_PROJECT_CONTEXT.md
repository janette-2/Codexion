# Codexion — dossier de contexto para estudio y defensa 42

> Documento de contexto para estudiar el proyecto con Claude. Describe el repositorio observado el 1 de noviembre de 2026. Si contradice una ejecución o una línea de código, prevalece la evidencia del repositorio.

## Cómo usar este archivo con Claude

Actúa como tutor socrático de 42, no como generador de soluciones. Explica primero la teoría y el por qué; no entregues implementaciones completas listas para copiar; usa pseudocódigo, invariantes y trazas pequeñas; localiza archivo y línea antes de proponer cambios; respeta Norminette, -Wall -Wextra -Werror -pthread y las funciones autorizadas; incluye casos límite y dos preguntas de defensa en cada bloque; distingue implementado, preparado, pendiente y verificado. Si digo MODO 42, vuelve a estas reglas.

## Qué es Codexion

Proyecto de concurrencia en C inspirado en los filósofos, con personas programadoras y dongles. Cada coder es un hilo y necesita dos dongles vecinos para compilar. Su ciclo es compilar -> depurar -> refactorizar. La simulación termina por burnout o cuando todos alcanzan number_of_compiles_required. fifo usa el ticket más antiguo; edf usa el deadline más próximo y ticket como desempate. FREE no implica disponible durante el cooldown.

## Argumentos

number_of_coders time_to_burnout time_to_compile time_to_debug time_to_refactor number_of_compiles_required dongle_cooldown scheduler

Se requieren exactamente ocho argumentos. Los números deben ser positivos, no vacíos, no negativos, numéricos y <= INT_MAX. scheduler debe ser fifo o edf. N=1 es válido: ambos ids apuntan al único dongle, por lo que el predicado debe rechazar esa pareja y el monitor debe detectar el burnout.

## Restricciones 42

Flags: -Wall -Wextra -Werror -pthread. No libft. Funciones autorizadas: pthread_create, pthread_join, pthread_mutex_init/lock/unlock/destroy, pthread_cond_init/wait/timedwait/broadcast/destroy, gettimeofday, usleep, write, malloc, free, printf, fprintf, strcmp, strlen, atoi y memset.

## Estructuras

t_coder: id, contador de compilaciones, last_compilation, pthread_t, control, dongles vecinos, ticket, waiting y heap_pos.

t_dongle: state_dongle, dongle_id y available_at. La disponibilidad es state == FREE y current_time() >= available_at.

t_queue: items, size y capacity; es un min-heap lógico.

t_control: argumentos, tiempos, arrays, monitor, mtx_dongles, mtx_log, cond_dongles, simulation_status y scheduler_queue. mtx_dongles protege recursos, cola y estado global; mtx_log serializa logs.

## Mapa de archivos

main.c: inicialización, cleanup y orquestación. Revisar created = 3 después de init_sync: explicar el contador y evitar números mágicos.

init.c: parseo y validación, incluyendo NULL, vacío, cero, negativos, caracteres inválidos y overflow.

coders.c: inicialización, pthread_create, fallos parciales, stop y join.

compiling.c: adquisición, dos logs de dongle, compile, devolución con cooldown, debug/refactor y rutina del coder.

dongles.c: array y topología circular; N=1 produce ids iguales.

resources.c: dongle_ready, disponibilidad, log serializado y espera con while + cond_wait/timedwait.

sync.c: init/destroy de mutexes y condición con cleanup parcial en orden inverso.

simulation.c: reloj en ms, conversión a timespec y monitor de burnout/fin.

queue.c y queue_aux.c: ticket, heap, elección electable y deferred.

heap_tools.c y heap_tools_aux.c: push, peek, pop, remove, swap, sift-up/down. Invariante: items[coder->heap_pos] == coder mientras esté dentro; fuera, heap_pos == -1.

## Flujo

main -> init_control -> check_args -> init_waiting_queue -> start_time -> init_coders -> init_dongles -> assignation -> init_sync -> run_simulation -> crear coders y monitor -> join -> destroy_sync -> free_control.

Una solicitud bloquea mtx_dongles, registra ticket una vez, entra en heap, espera en un while hasta ser el mejor candidato elegible, marca ambos dongles BLOCKED, actualiza last_compilation, sale del heap, pone waiting = NOT_WAITING y desbloquea.

## Invariantes de defensa

1. Todo cambio de recursos ocurre bajo mtx_dongles. 2. Nunca se duerme con ese mutex retenido. 3. No hay duplicados en la cola. 4. Burnout limpia waiting y heap_pos. 5. Toda condición se revisa en while. 6. Cada log está protegido por mtx_log. 7. Join antes de destroy/free. 8. FREE y available_at se actualizan juntos. 9. FIFO compara tickets y EDF deadlines con desempate. 10. N=1 no adquiere dos veces el mismo dongle.

## Riesgos a auditar

No afirmar cierre sin pruebas actuales: created = 3 fijo; fallos parciales de pthread_create; que el scheduler gobierne realmente la concesión; drain_unserviceable/deferred/heap_pos; protección de campos compartidos; limite > frente a >= en burnout; retraso del monitor y buffering de printf; repetir build, Norma, FIFO, EDF, N=1, cooldown alto y Valgrind.

## Pruebas

Argumentos: aridad, 0, -1, +1, vacío, abc, 12abc, 2147483647, 2147483648 y schedulers inválidos. Concurrencia: N=1, N=2, N>=3, cooldown 0/pequeño/grande, compile largo y burnout corto. Scheduler: FIFO, EDF, empate EDF y candidato alternativo. Fallos: malloc de cada estructura, cada primitive de sync, coder intermedio, monitor y Valgrind.

## Preguntas de defensa

Por qué un mutex único evita adquisición simultánea; por qué no se mantiene durante usleep; qué ocurre si left_id == right_id; diferencia entre FREE y available_at; qué es spurious wakeup; diferencia entre cond_wait y timedwait; por qué broadcast después de actualizar; complejidad del heap; por qué actualizar heap_pos; por qué desempatar EDF; significado de last_compilation; cómo cumplir burnout <10 ms; qué es fallo parcial de pthread_create; qué destruir si falla la tercera primitive.

## Comandos

make fclean && make

norminette coders/*.c coders/*.h

./codexion 3 1000 100 50 50 2 30 fifo

./codexion 3 1000 100 50 50 2 30 edf

timeout 8 stdbuf -oL ./codexion 1 10 10 10 10 2 10 edf

## Documentación relacionada

Codexion.md subject; CONTROL.md hitos y estado histórico; TEORIA.md fundamentos; DIAGRAMAS.md arquitectura; GUIA_ESTUDIO.md recorrido; REPASO.md defensa; README.md entrega; AGENTS.md tutoría.

## Objetivo final

Debo poder explicar estado global, mutex por campo, ciclo de una solicitud, comparación FIFO/EDF, cooldown temporizado, monitor, terminación y cleanup de éxitos y fallos. Si no puedo responder, hacer una micro-sesión de teoría y una prueba concreta, no una reescritura completa.
