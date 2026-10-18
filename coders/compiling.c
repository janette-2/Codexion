/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   compiling.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/11 20:39:17 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/18 23:56:10 by janrodri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	block_dongles(t_coder *coder)
{
	pthread_mutex_lock(&coder->control->mtx_dongles);
	while (dongles_available(
			coder->control, coder->left_dongle, coder->right_dongle) == 0)
	{
		pthread_cond_wait(
			&coder->control->cond_dongles, &coder->control->mtx_dongles);
	}
	coder->control->dongles[
		coder->left_dongle - 1].state_dongle = DONGLE_BLOCKED;
	coder->control->dongles[
		coder->right_dongle - 1].state_dongle = DONGLE_BLOCKED;
	pthread_mutex_unlock(&coder->control->mtx_dongles);
	return (1);
}

void	compiling_phase(t_coder *coder)
{
	if (block_dongles(coder) == 1)
	{
		log_line(coder->control, LOG_COMPILING, coder->id);
		coder->last_compilation = current_time();
		usleep(coder->control->time_to_compile * 1000);
		pthread_mutex_lock(&coder->control->mtx_dongles);
		coder->number_of_compiles_done++;
		coder->control->dongles[
			coder->right_dongle - 1].state_dongle = DONGLE_FREE;
		coder->control->dongles[coder->right_dongle - 1].available_at = (
				current_time() + coder->control->dongle_cooldown);
		coder->control->dongles[
			coder->left_dongle - 1].state_dongle = DONGLE_FREE;
		coder->control->dongles[coder->left_dongle - 1].available_at = (
				current_time() + coder->control->dongle_cooldown);
		pthread_mutex_unlock(&coder->control->mtx_dongles);
	}
}

void	debugging_phase(t_coder *coder)
{
	log_line(coder->control, LOG_DEBUGGING, coder->id);
	usleep(coder->control->time_to_debug * 1000);
}

void	refactoring_phase(t_coder *coder)
{
	log_line(coder->control, LOG_REFACTORING, coder->id);
	usleep(coder->control->time_to_refactor * 1000);
}

void	*coder_routine(void *arg)
{
	t_coder	*coder;
	int		compiles_required;

	coder = (t_coder *)arg;
	compiles_required = coder->control->number_of_compiles_required;
	while (coder->number_of_compiles_done < compiles_required)
	{
		compiling_phase(coder);
		debugging_phase(coder);
		refactoring_phase(coder);
	}
	return (NULL);
}
