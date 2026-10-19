/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/07 19:17:11 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/19 21:55:08 by janrodri         ###   ########.fr       */
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
# define DONGLE_COOLDOWN 2

# define LOG_TAKEN "has taken a dongle"
# define LOG_COMPILING "is compiling"
# define LOG_DEBUGGING "is debugging"
# define LOG_REFACTORING "is refactoring"
# define LOG_BURNEDOUT "burned out"

typedef struct s_control	t_control;
typedef struct s_coder		t_coder;
typedef struct s_dongle		t_dongle;

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
};

struct s_dongle
{
	int			state_dongle; //0 for blocked, 1 for free, 2 for cooldown
	int			dongle_id;
	long long	available_at;
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
	t_coder			*coders;
	t_dongle		*dongles;
	pthread_mutex_t	mtx_dongles;
	pthread_mutex_t	mtx_log;
	pthread_cond_t	cond_dongles;
};

/* INIT.C */
int			check_args(int argc, char **arg, t_control *control);

/* DONGLES.C */
void		dongles_args(t_dongle *dongle, int i);
int			init_dongles(t_control *control);
void		dongles_assignation(t_control *control);
void		cooldown_available_time_dongle(
				t_dongle *dongle, t_control *control);

/* CODERS.C */
void		coder_args(t_coder *coder, int i, t_control *control);
int			init_coders(t_control *control);
int			run_simulation(t_control *control);

/* SIMULATION.C */
long long	current_time(void);
void		ms_to_timespec(long long ms, struct timespec *when);

/* SYNC.C */
int			init_sync(t_control *control);
void		destroy_sync(t_control *control, int created);

/* RESOURCES.C */
int			dongles_available(t_control *control, int left_id, int right_id);
int			dongle_ready(t_dongle *dongle);
void		log_line(t_control *control, const char *message, int id);

/* COMPILING.C */
void		block_dongles(t_coder *coder);
void		compiling_phase(t_coder *coder);
void		debugging_phase(t_coder *coder);
void		refactoring_phase(t_coder *coder);
void		*coder_routine(void *arg);

#endif
