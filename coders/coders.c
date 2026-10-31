/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coders.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/14 16:24:55 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/31 20:56:36 by janrodri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	coder_args(t_coder *coder, int i, t_control *control)
{
	coder->id = i;
	coder->number_of_compiles_done = 0;
	coder->last_compilation = control->start_time;
	coder->control = control;
	coder->left_dongle = 0;
	coder->right_dongle = 0;
	coder->ticket = -1;
	coder->waiting = REQUEST_NOT_WAITING;
	coder->heap_pos = -1;
}

int	start_coders(t_control *control)
{
	int	created;
	int	result;

	created = 0;
	while (created < control->number_of_coders)
	{
		result = pthread_create(&control->coders[created].coder_pthread, NULL,
				coder_routine, &control->coders[created]);
		if (result != 0)
			break ;
		created++;
	}
	return (created);
}

void	stop_and_join(t_control *control, int created)
{
	int	i;

	pthread_mutex_lock(&control->mtx_dongles);
	control->simulation_status = SIM_STOPPED;
	pthread_cond_broadcast(&control->cond_dongles);
	pthread_mutex_unlock(&control->mtx_dongles);
	i = 0;
	while (i < created)
		pthread_join(control->coders[i++].coder_pthread, NULL);
}

int	init_coders(t_control *control)
{
	int	i;

	control->coders = malloc(sizeof(t_coder) * control->number_of_coders);
	if (!control->coders)
	{
		control->coders = NULL;
		return (-1);
	}
	i = 0;
	while (i < control->number_of_coders)
	{
		coder_args(&(control->coders[i]), i + 1, control);
		i++;
	}
	return (0);
}

int	run_simulation(t_control *control)
{
	int	created;
	int	result_monitor;
	int	j;

	created = start_coders(control);
	if (created != control->number_of_coders)
	{
		stop_and_join(control, created);
		return (1);
	}
	result_monitor = pthread_create(&control->monitor_pthread,
			NULL, monitor_routine, control);
	if (result_monitor != 0)
	{
		stop_and_join(control, created);
		return (1);
	}
	j = 0;
	while (j < created)
		pthread_join(control->coders[j++].coder_pthread, NULL);
	pthread_join(control->monitor_pthread, NULL);
	return (0);
}
