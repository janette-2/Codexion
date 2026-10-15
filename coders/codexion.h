/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/07 19:17:11 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/15 16:50:39 by janrodri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CODEXION_H
# define CODEXION_H

# include <pthread.h>
# include <unistd.h>
# include <stdio.h>
# include <string.h>
# include <stdlib.h>

typedef struct s_control	t_control;
typedef struct s_coder		t_coder;
typedef struct s_dongle		t_dongle;

struct s_coder
{
	int			id;
	int			number_of_compiles_done;
	int			last_compilation;
	pthread_t	coder_pthread;
	t_control	*control;
	// Everyone access to the same control variable through a pointer
	int			left_dongle;
	int			right_dongle;
};

struct s_dongle
{
	int		state_dongle; //0 for blocked, 1 for free, 2 for cooldown
	int		dongle_id;
	int		available_at;
};

struct s_control
{
	int			number_of_coders;
	char		*scheduler;
	int			number_of_compiles_required;
	int			time_to_burnout;
	int			time_to_compile;
	int			time_to_debug;
	int			time_to_refactor;
	int			dongle_cooldown;
	int			simulation_status;
	// 2 for finished, 1 for running, 0 for stopped
	t_coder		*coders;
	t_dongle	*dongles;
};

/* INIT.C */
int		check_args(int argc, char **arg, t_control *control);

/* DONGLES.C */
void	dongles_args(t_dongle *dongle, int i);
int		init_dongles(t_control *control);
void	dongles_assignation(t_control *control);

/* CODERS.C */
void	coder_args(t_coder *coder, int i, t_control *control);
int		init_coders(t_control *control);

#endif
