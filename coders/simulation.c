/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   simulation.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 13:54:41 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/22 21:32:48 by janrodri         ###   ########.fr       */
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

void	stop_simulation(t_control *control, int coder_id)
{
	pthread_mutex_lock(&(control->mtx_dongles));
	log_line(control, LOG_BURNEDOUT, coder_id);
	control->simulation_status = SIM_STOPPED;
	pthread_cond_broadcast(&(control->cond_dongles));
	pthread_mutex_unlock(&(control->mtx_dongles));
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
			if (current_time() - (control->coders[i].last_compilation)
				> control->time_to_burnout)
			{
				stop_simulation(control, control->coders[i].id);
				break ;
			}
			if (control->coders[i++].number_of_compiles_done
				== control->number_of_compiles_required)
				finish_coders++;
			if (finish_coders == control->number_of_coders)
				control->simulation_status = SIM_FINISHED;
		}
		usleep(5000);
	}
	return (NULL);
}
