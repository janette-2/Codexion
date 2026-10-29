/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/07 19:17:11 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/29 20:15:47 by janrodri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CODEXION_H
# define CODEXION_H

# include <pthread.h>
# include <unistd.h>
# include <stdio.h>
# include <string.h>
# include <stdlib.h>
# include <sys/time.h>

# define DONGLE_BLOCKED 0
# define DONGLE_FREE 1

# define LOG_TAKEN "has taken a dongle"
# define LOG_COMPILING "is compiling"
# define LOG_DEBUGGING "is debugging"
# define LOG_REFACTORING "is refactoring"
# define LOG_BURNEDOUT "burned out"

# define DEADLINE_NEVER  -1

# define SIM_RUNNING 1
# define SIM_STOPPED 2
# define SIM_FINISHED 3

# define REQUEST_NOT_WAITING 0
# define REQUEST_WAITING 1

typedef struct s_control	t_control;
typedef struct s_coder		t_coder;
typedef struct s_dongle		t_dongle;
typedef struct s_queue		t_queue;

struct s_coder
{
	int				id;
	int				number_of_compiles_done;
	long long		last_compilation;
	pthread_t		coder_pthread;
	t_control		*control;
	// Everyone access to the same control variable through a pointer
	int				left_dongle;
	int				right_dongle;
	long long		ticket;
	int				waiting;
	int				heap_pos;
};

struct s_dongle
{
	int			state_dongle; //DONGLE_BLOCKED or DONGLE_FREE
	int			dongle_id;
	long long	available_at; //cooldown instant; dongle is FREE but not ready
};

struct s_queue
{
	t_coder		**items;
	int			size;
	int			capacity;
};

struct s_control
{
	int				number_of_coders;
	char			*scheduler;
	int				number_of_compiles_required;
	int				time_to_burnout;
	int				time_to_compile;
	int				time_to_debug;
	int				time_to_refactor;
	int				dongle_cooldown;
	int				simulation_status;
	long long		start_time;
	pthread_t		monitor_pthread;
	t_coder			*coders;
	t_dongle		*dongles;
	pthread_mutex_t	mtx_dongles;
	pthread_mutex_t	mtx_log;
	pthread_cond_t	cond_dongles;
	t_queue			scheduler_queue;
	long long		next_ticket;
	t_coder			**deferred;
};

/* INIT.C */
int			check_args(int argc, char **arg, t_control *control);

/* DONGLES.C */
void		dongles_args(t_dongle *dongle, int i);
int			init_dongles(t_control *control);
void		dongles_assignation(t_control *control);
void		put_dongles_blocked(t_coder *coder);

/* CODERS.C */
void		coder_args(t_coder *coder, int i, t_control *control);
int			init_coders(t_control *control);
int			run_simulation(t_control *control);

/* SIMULATION.C */
long long	current_time(void);
void		ms_to_timespec(long long ms, struct timespec *when);
void		*monitor_routine(void *arg);

/* SYNC.C */
int			init_sync(t_control *control);
void		destroy_sync(t_control *control, int created);

/* RESOURCES.C */
int			dongles_available(t_control *control, int left_id, int right_id);
int			dongle_ready(t_dongle *dongle);
void		log_line(t_control *control, const char *message, int id);
long long	cooldowns_deadline(t_control *control, int left_id, int right_id);
void		wait_for_dongles(t_coder *coder);

/* COMPILING.C */
void		block_dongles(t_coder *coder);
void		compiling_phase(t_coder *coder);
void		debugging_phase(t_coder *coder);
void		refactoring_phase(t_coder *coder);
void		*coder_routine(void *arg);

/* QUEUE.C */
void		register_request(t_coder *coder);
void		enqueue_coder(t_control *control, t_coder *coder);
int			temp_beats_best(t_coder *temp, t_coder *best, t_control *control);
t_coder		*choose_next_in_queue(t_control *control);
long long	deadline(t_coder *coder);

/* QUEUE_AUX.C */
int			init_waiting_queue(t_control *control);
void		dequeue_coder(t_coder *coder);

/* HEAP_TOOLS.C */
int			heap_better(t_coder *a, t_coder *b, t_control *control);
void		heap_swap(t_coder *a, t_coder *b, t_control *control);

#endif
