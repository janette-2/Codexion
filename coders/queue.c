/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   queue.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/24 19:46:58 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/30 18:51:57 by janrodri         ###   ########.fr       */
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
	heap_push(control, coder);
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
	{
		if (deadline(temp) == deadline(best))
			return (temp->ticket < best->ticket);
		return (deadline(temp) < deadline(best));
	}
}

t_coder	*choose_next_in_queue(t_control *control)
{
	t_coder	*best;
	int		i;

	i = drain_unserviceable(control);
	best = heap_peek(control);
	restore_deferred(control, i);
	return (best);
}
