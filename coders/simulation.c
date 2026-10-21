/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   simulation.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 13:54:41 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/21 18:45:40 by janrodri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

long long	current_time(void)
{
	struct timeval	time;

	gettimeofday(&time, NULL);
	return ((time.tv_sec * 1000) + time.tv_usec / 1000);
}

void	ms_to_timespec(long long ms, struct timespec *when)
{
	when->tv_sec = ms / 1000;
	when->tv_nsec = (ms % 1000) * 1000000;
}

void	*monitor_routine(void *arg)
{
	t_control	*control;
	int			i;
	int			finish_coders;

	control = (t_control *)arg;
	while (control->simulation_status == SIM_RUNNING)
	{
		i = 0;
		finish_coders = 0;
		while (i < control->number_of_coders)
		{
			if (control->coders[i++].number_of_compiles_done
				== control->number_of_compiles_required)
				finish_coders++;
			if (finish_coders == control->number_of_coders)
				control->simulation_status = SIM_FINISHED;
		}
	}
	return (NULL);
}
