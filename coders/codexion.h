/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/07 19:17:11 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/12 19:07:08 by janrodri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CODEXION_H
# define CODEXION_H

# include <pthread.h>
# include <unistd.h>
# include <stdio.h>
# include <string.h>
# include <stdlib.h>

// MTER AQUI TODOS LOS INCLUDE Y SOLO PASAR ESTE HEADER

typedef struct s_control
{
	int		number_of_coders;
	char	*scheduler;
	int		number_of_compiles_required;
	int		time_to_burnout;
	int		time_to_compile;
	int		time_to_debug;
	int		time_to_refactor;
	int		dongle_cooldown;
	int		simulation_status; // 1 for running, 0 for stopped
}	t_control;

typedef struct s_coder
{
	int			id;
	int			number_of_compiles_done;
	int			last_compilation;
	pthread_t	coder_pthread;
	t_control	*control;
	// Everyone access to the same control variable through a pointer
	int			left_dongle;
	int			right_dongle;
}	t_coder;

typedef struct s_dongle
{
	int		state_dongle; //0 for blocked, 1 for free, 2 for cooldown
	int		dongle_id;
}	t_dongle;

/* INIT.C */
int	check_args(int argc, char **arg);

#endif
