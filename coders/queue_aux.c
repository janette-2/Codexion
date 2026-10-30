/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   queue_aux.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/25 15:31:41 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/30 18:45:32 by janrodri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	init_waiting_queue(t_control *control)
{
	control->scheduler_queue.items = malloc(
			sizeof(t_coder *) * control->number_of_coders);
	if (!control->scheduler_queue.items)
	{
		control->scheduler_queue.items = NULL;
		control->scheduler_queue.size = 0;
		control->scheduler_queue.capacity = 0;
		return (1);
	}
	control->scheduler_queue.size = 0;
	control->scheduler_queue.capacity = control->number_of_coders;
	control->deferred = malloc(
			sizeof(t_coder *) * control->number_of_coders);
	if (!control->deferred)
	{
		free(control->scheduler_queue.items);
		control->scheduler_queue.items = NULL;
		control->scheduler_queue.size = 0;
		control->scheduler_queue.capacity = 0;
		return (1);
	}
	return (0);
}

void	dequeue_coder(t_coder *coder)
{
	heap_remove(coder->control, coder);
}

int	coder_electable(t_coder *coder)
{
	if (!coder)
		return (0);
	if (coder->waiting != REQUEST_WAITING)
		return (0);
	return (dongles_available(coder->control,
			coder->left_dongle, coder->right_dongle));
}

int	drain_unserviceable(t_control *control)
{
	int	i;

	i = 0;
	while (control->scheduler_queue.size > 0
		&& !coder_electable(heap_peek(control)))
	{
		control->deferred[i] = heap_pop(control);
		i++;
	}
	return (i);
}

void	restore_deferred(t_control *control, int i)
{
	while (i > 0)
	{
		i--;
		heap_push(control, control->deferred[i]);
	}
}
