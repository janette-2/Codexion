/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   queue.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/24 19:46:58 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/25 12:56:31 by janrodri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	init_waiting_queue(t_control *control)
{
	control->waiting_queue = malloc(
			sizeof(t_coder *) * control->number_of_coders);
	if (!control->waiting_queue)
	{
		control->waiting_queue = NULL;
		return (1);
	}
	return (0);
}

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
