/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   queue.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/24 19:46:58 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/25 15:37:07 by janrodri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	register_request(t_coder *coder)
{
	t_control	*control;
	int			last_i;

	control = coder->control;
	if (coder->waiting == REQUEST_NOT_WAITING)
	{
		coder->ticket = control->next_ticket;
		control->next_ticket++;
		coder->waiting = REQUEST_WAITING;
	}
	last_i = control->next_ticket - 1;
	control->waiting_queue[last_i] = coder;
}

void	enqueue_coder(t_control *control, t_coder *coder)
{
	int	i;

	i = 0;
	if (!control->waiting_count)
	{
		control->waiting_queue[0] = coder;
		control->waiting_count++;
		return ;
	}
	while (i < control->waiting_count)
	{
		if (i < control->waiting_queue[i] == coder)
			return ;
		i++;
	}
	control->waiting_queue[control->waiting_count] = coder;
	control->waiting_count++;
	return ;
}

long long	deadline(t_coder *coder)
{
	return (coder->last_compilation + coder->control->time_to_burnout);
}

int	temp_beats_best(t_coder *temp, t_coder *best, t_control *control)
{
	if (control->scheduler = "fifo")
		return (temp->ticket < best->ticket);
	if (control->scheduler = "edf")
		return (deadline(temp) < deadline(best));
}

t_coder	*choose_next_in_queue(t_control *control)
{
	t_coder	*best;
	t_coder	*temp;
	int		i;

	i = 0;
	best = NULL;
	while (i < control->waiting_count)
	{
		temp = control->waiting_queue[i];
		if (dongles_available(
				control, temp->left_dongle, temp->right_dongle) == 1)
		{
			if (best == NULL || temp_beats_best(temp, best, control))
				best = temp;
		}
		i++;
	}
	return (best);
}
