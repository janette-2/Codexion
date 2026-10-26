/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   queue.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/24 19:46:58 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/26 22:31:25 by janrodri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	register_request(t_coder *coder)
{
	t_control	*control;

	control = coder->control;
	if (coder->waiting == REQUEST_NOT_WAITING)
	{
		coder->ticket = control->next_ticket;
		control->next_ticket++;
		coder->waiting = REQUEST_WAITING;
	}
}

void	enqueue_coder(t_control *control, t_coder *coder)
{
	int	i;

	i = 0;
	if (control->scheduler_queue.size >= control->scheduler_queue.capacity)
		return ;
	if (!control->scheduler_queue.size)
	{
		control->scheduler_queue.items[0] = coder;
		control->scheduler_queue.size++;
		return ;
	}
	while (i < control->scheduler_queue.size)
	{
		if (control->scheduler_queue.items[i] == coder)
			return ;
		i++;
	}
	control->scheduler_queue.items[control->scheduler_queue.size] = coder;
	control->scheduler_queue.size++;
	return ;
}

long long	deadline(t_coder *coder)
{
	return (coder->last_compilation + coder->control->time_to_burnout);
}

int	temp_beats_best(t_coder *temp, t_coder *best, t_control *control)
{
	if (strcmp(control->scheduler, "fifo") == 0)
		return (temp->ticket < best->ticket);
	else
		return (deadline(temp) < deadline(best));
}

t_coder	*choose_next_in_queue(t_control *control)
{
	t_coder	*best;
	t_coder	*temp;
	int		i;

	i = 0;
	best = NULL;
	while (i < control->scheduler_queue.size)
	{
		temp = control->scheduler_queue.items[i];
		if (dongles_available(
				control, temp->left_dongle, temp->right_dongle) == 1)
		{
			if (temp->waiting == REQUEST_NOT_WAITING)
			{
				i++;
				continue ;
			}
			if (best == NULL || temp_beats_best(temp, best, control))
				best = temp;
		}
		i++;
	}
	return (best);
}
