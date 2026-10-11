/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/07 19:17:11 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/11 19:58:16 by janrodri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CODEXION_H
# define CODEXION_H

# include <pthread.h>

typedef struct s_coder
{
	int			id;
	int			number_of_compiles_done;
	pthread_t	coder_pthread;
	t_control	control;
}	t_coder;

typedef struct s_dongle
{
	int		state_dongle; //0 for blocked, 1 for free, 2 for cooldown
	int		dongle_id;
}	t_dongle;

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

#endif
