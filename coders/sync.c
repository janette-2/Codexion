/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sync.c                                             :+:      :+:    :+:   */
/*                                                    :::                   */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/17 20:30:00 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/17 20:30:00 by janrodri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static void	log_sync_error(int step)
{
	if (step == 1)
		printf("Error. The initialization of 'mtx_dongles' failed.\n");
	if (step == 2)
		printf("Error. The initialization of 'mtx_log' failed.\n");
	if (step == 3)
		printf("Error. The initialization of 'cond_dongles' failed.\n");
}

void	destroy_sync(t_control *control, int created)
{
	if (created >= 3)
		pthread_cond_destroy(&(control->cond_dongles));
	if (created >= 2)
		pthread_mutex_destroy(&(control->mtx_log));
	if (created >= 1)
		pthread_mutex_destroy(&(control->mtx_dongles));
}

int	init_sync(t_control *control)
{
	int	created;

	created = 0;
	if (pthread_mutex_init(&(control->mtx_dongles), NULL) != 0)
	{
		log_sync_error(1);
		destroy_sync(control, created);
		return (1);
	}
	created++;
	if (pthread_mutex_init(&(control->mtx_log), NULL) != 0)
	{
		log_sync_error(2);
		destroy_sync(control, created);
		return (2);
	}
	created++;
	if (pthread_cond_init(&(control->cond_dongles), NULL) != 0)
	{
		log_sync_error(3);
		destroy_sync(control, created);
		return (3);
	}
	created++;
	return (0);
}

int	simulation_is_running(t_control *control)
{
	int	running;

	pthread_mutex_lock(&control->mtx_dongles);
	running = (control->simulation_status == SIM_RUNNING);
	pthread_mutex_unlock(&(control->mtx_dongles));
	return (running);
}
