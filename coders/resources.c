/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   resources.c                                        :+:      :+:    :+:   */
/*                                                   | |                  */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   #+           */
/*   Created: 2026/10/15 16:57:14 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/17 20:35:00 by janrodri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	dongle_ready(t_dongle *dongle)
{
	if (dongle->state_dongle != DONGLE_FREE)
		return (0);
	if (current_time() < dongle->available_at)
		return (0);
	return (1);
}

int	dongles_available(t_control *control, int left_id, int right_id)
{
	if (left_id == right_id)
		return (0);
	if (!dongle_ready(&(control->dongles[left_id - 1])))
		return (0);
	if (!dongle_ready(&(control->dongles[right_id - 1])))
		return (0);
	return (1);
}

void	log_line(t_control *control, const char *message, int id)
{
	int	elapsed;

	elapsed = (int)(current_time() - control->start_time);
	pthread_mutex_lock(&(control->mtx_log));
	printf("%d %d %s\n", elapsed, id, message);
	pthread_mutex_unlock(&(control->mtx_log));
}
