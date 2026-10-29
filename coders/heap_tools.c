/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap_tools.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/29 19:57:28 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/29 21:27:48 by janrodri         ###   ########.fr       */
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

void	heap_up(t_control *control, int i)
{
	t_queue	*q;
	int		parent;

	q = &control->scheduler_queue;
	if (i < 0 || i >= q->size)
		return ;
	while (i > 0)
	{
		parent = (i - 1) / 2;
		if (!heap_better(q->items[i], q->items[parent], control))
			break ;
		else
			heap_swap(q->items[i], q->items[parent], control);
		i = parent;
	}
}

void	heap_down(t_control *control, int i)
{
	t_queue	*q;
	int		left;
	int		right;
	int		best;

	q = &control->scheduler_queue;
	if (i < 0 || i >= q->size)
		return ;
	left = 2 * i + 1;
	while (left < q->size)
	{
		right = left + 1;
		best = left;
		if (right < q->size && heap_better(
				q->items[right], q->items[left], control))
			best = right;
		if (!heap_better(q->items[best], q->items[i], control))
			break ;
		heap_swap(q->items[best], q->items[i], control);
		i = best;
		left = 2 * i + 1;
	}
}
