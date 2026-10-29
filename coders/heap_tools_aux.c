/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap_tools_aux.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/29 21:28:18 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/29 22:13:56 by janrodri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	heap_push(t_control *control, t_coder *coder)
{
	t_queue	*q;

	q = &control->scheduler_queue;
	if (coder->heap_pos != -1 || q->size >= q->capacity)
		return ;
	q->items[q->size] = coder;
	coder->heap_pos = q->size;
	q->size++;
	heap_up(control, q->size - 1);
}

t_coder	*heap_peek(t_control *control)
{
	t_queue	*q;

	q = &control->scheduler_queue;
	if (q->size == 0)
		return (NULL);
	else
		return (q->items[0]);
}

t_coder	*heap_pop(t_control *control)
{
	t_queue	*q;
	t_coder	*root;

	q = &control->scheduler_queue;
	if (q->size == 0)
		return (NULL);
	root = q->items[0];
	q->size--;
	if (q->size > 0)
	{
		q->items[0] = q->items[q->size];
		q->items[0]->heap_pos = 0;
		heap_down(control, 0);
	}
	q->items[q->size] = NULL;
	root->heap_pos = -1;
	return (root);
}

static void	heap_delete_at(t_control *control, int idx)
{
	t_queue	*q;
	t_coder	*last;

	q = &control->scheduler_queue;
	q->size--;
	if (idx == q->size)
	{
		q->items[idx] = NULL;
		return ;
	}
	last = q->items[q->size];
	q->items[idx] = last;
	last->heap_pos = idx;
	q->items[q->size] = NULL;
	if (idx > 0 && heap_better(last, q->items[(idx - 1) / 2], control))
		heap_up(control, idx);
	else
		heap_down(control, idx);
}

void	heap_remove(t_control *control, t_coder *coder)
{
	t_queue	*q;
	int		idx;

	q = &control->scheduler_queue;
	if (coder == NULL || coder->heap_pos < 0 || coder->heap_pos >= q->size)
		return ;
	idx = coder->heap_pos;
	coder->heap_pos = -1;
	heap_delete_at(control, idx);
}
