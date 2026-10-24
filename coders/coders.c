/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coders.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/14 16:24:55 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/24 19:44:30 by janrodri         ###   ########.fr       */
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
}

int	init_coders(t_control *control)
{
	int	i;

	control->coders = malloc(sizeof(t_coder) * control->number_of_coders);
	if (!control->coders)
	{
		free(control->coders);
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

// Nº of created threads == i;
int	run_simulation(t_control *control)
{
	int	i;
	int	j;

	i = 0;
	while (i < control->number_of_coders)
	{
		if (pthread_create(
				&control->coders[i].coder_pthread, NULL,
				coder_routine, &control->coders[i]) != 0)
			break ;
		i++;
	}
	pthread_create(&control->monitor_pthread, NULL, monitor_routine, control);
	j = 0;
	while (j < i)
		pthread_join(control->coders[j++].coder_pthread, NULL);
	pthread_join(control->monitor_pthread, NULL);
	if (i == control->number_of_coders)
		return (0);
	return (1);
}
