/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   compiling.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/11 20:39:17 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/18 20:11:52 by janrodri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	compiling_phase(t_coder *coder)
{
	int	happy_path;

	happy_path = 0;
	pthread_mutex_lock(&coder->control->mtx_dongles);
	if (dongles_available(
			coder->control, coder->left_dongle, coder->right_dongle) == 1)
	{
		happy_path = 1;
		log_line(coder->control, LOG_COMPILING, coder->id);
		coder->control->dongles[
			coder->left_dongle - 1].state_dongle = DONGLE_BLOCKED;
		coder->control->dongles[coder->left_dongle - 1].available_at = (
				current_time() + coder->control->dongle_cooldown);
		coder->control->dongles[
			coder->right_dongle - 1].state_dongle = DONGLE_BLOCKED;
		coder->control->dongles[coder->right_dongle - 1].available_at = (
				current_time() + coder->control->dongle_cooldown);
	}
	pthread_mutex_unlock(&coder->control->mtx_dongles);
	if (happy_path)
	{
		coder->last_compilation = current_time();
		usleep(coder->control->time_to_compile * 1000);
		pthread_mutex_lock(&coder->control->mtx_dongles);
		coder->number_of_compiles_done++;
		coder->control->dongles[
			coder->right_dongle - 1].state_dongle = DONGLE_FREE;
		coder->control->dongles[
			coder->left_dongle - 1].state_dongle = DONGLE_FREE;
		pthread_mutex_unlock(&coder->control->mtx_dongles);
	}
}

void	debugging_phase(t_coder *coder)
{
	
}

void	refactoring_phase(t_coder *coder)
{
	
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
