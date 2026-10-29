/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   queue_aux.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/25 15:31:41 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/29 20:18:21 by janrodri         ###   ########.fr       */
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
		control->scheduler_queue.items = NULL;
		control->scheduler_queue.size = 0;
		control->scheduler_queue.capacity = 0;
		return (free(control->scheduler_queue.items), 1);
	}
	return (0);
}

void	dequeue_coder(t_coder *coder)
{
	t_queue	*queue;
	int		i;

	queue = &coder->control->scheduler_queue;
	i = 0;
	while (i < queue->size && queue->items[i] != coder)
		i++;
	if (i == queue->size)
		return ;
	queue->items[i] = queue->items[queue->size - 1];
	queue->size--;
	queue->items[queue->size] = NULL;
}
