/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap_tools.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/29 19:57:28 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/29 20:14:50 by janrodri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	heap_better(t_coder *a, t_coder *b, t_control *control)
{
	return (temp_beats_best(a, b, control));
}

void	heap_swap(t_coder *a, t_coder *b, t_control *control)
{
	t_coder	*dir_a;
	t_coder	*dir_b;
	int		pos_a;
	int		pos_b;

	pos_a = a->heap_pos;
	pos_b = b->heap_pos;
	dir_a = control->scheduler_queue.items[pos_a];
	dir_b = control->scheduler_queue.items[pos_b];
	control->scheduler_queue.items[pos_a] = dir_b;
	control->scheduler_queue.items[pos_b] = dir_a;
	a->heap_pos = pos_b;
	b->heap_pos = pos_a;
}
