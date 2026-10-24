/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   queue.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/24 19:46:58 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/24 20:59:43 by janrodri         ###   ########.fr       */
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

	control = coder->control;
	if (coder->waiting == REQUEST_NOT_WAITING)
	{
		coder->ticket = control->next_ticket;
		control->next_ticket++;
		coder->waiting = REQUEST_WAITING;
	}
}
