/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   queue_aux.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/25 15:31:41 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/25 15:32:39 by janrodri         ###   ########.fr       */
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
