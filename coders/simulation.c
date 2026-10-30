/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   simulation.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 13:54:41 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/30 21:02:15 by janrodri         ###   ########.fr       */
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

int	scan_coders_locked(t_control *control, int *finish_coders)
{
	int	i;

	i = 0;
	*finish_coders = 0;
	while (i < control->number_of_coders)
	{
		if (current_time() - (control->coders[i].last_compilation)
			> control->time_to_burnout)
			return (i);
		if (control->coders[i].number_of_compiles_done
			== control->number_of_compiles_required)
			(*finish_coders)++;
		i++;
	}
	return (-1);
}

void	check_simulation_state(t_control *control)
{
	int			finish_coders;
	int			burned;

	pthread_mutex_lock(&(control->mtx_dongles));
	burned = scan_coders_locked(control, &finish_coders);
	if (burned != -1)
	{
		log_line(control, LOG_BURNEDOUT, control->coders[burned].id);
		control->simulation_status = SIM_STOPPED;
		pthread_cond_broadcast(&(control->cond_dongles));
	}
	else
	{
		if (finish_coders == control->number_of_coders)
		{
			control->simulation_status = SIM_FINISHED;
			pthread_cond_broadcast(&(control->cond_dongles));
		}
	}
	pthread_mutex_unlock(&(control->mtx_dongles));
}

void	*monitor_routine(void *arg)
{
	t_control	*control;

	control = (t_control *)arg;
	while (simulation_is_running(control))
	{
		check_simulation_state(control);
		usleep(5000);
	}
	return (NULL);
}
