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

long long	cooldowns_deadline(t_control *control, int left_id, int right_id)
{
	long long	at_left;
	long long	at_right;
	int			state_left;
	int			state_right;

	at_left = control->dongles[left_id - 1].available_at;
	at_right = control->dongles[right_id - 1].available_at;
	state_left = control->dongles[left_id - 1].state_dongle;
	state_right = control ->dongles[right_id - 1].state_dongle;
	if (left_id == right_id)
		return (DEADLINE_NEVER);
	if (state_left != DONGLE_FREE || state_right != DONGLE_FREE)
		return (DEADLINE_NEVER);
	else
	{
		if (at_left > at_right)
			return (at_left);
		else
			return (at_right);
	}
}

void	wait_for_dongles(t_coder *coder)
{
	long long		deadline_cool;
	struct timespec	when;

	while (coder->control->simulation_status == SIM_RUNNING
		&& (dongles_available(
				coder->control, coder->left_dongle, coder->right_dongle) == 0
			|| choose_next_in_queue(coder->control) != coder))
	{
		deadline_cool = cooldowns_deadline(coder->control,
				coder->left_dongle, coder->right_dongle);
		if (deadline_cool == DEADLINE_NEVER || deadline_cool <= current_time())
		{
			pthread_cond_wait(
				&coder->control->cond_dongles, &coder->control->mtx_dongles);
		}
		else
		{
			ms_to_timespec(deadline_cool, &when);
			pthread_cond_timedwait(&coder->control->cond_dongles,
				&coder->control->mtx_dongles, &when);
		}
	}
}
