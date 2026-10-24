/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/29 17:52:45 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/24 20:24:47 by janrodri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	free_control(t_control *control)
{
	if (control->coders)
	{
		free(control->coders);
		control->coders = NULL;
	}
	if (control->dongles)
	{
		free(control->dongles);
		control->dongles = NULL;
	}
	if (control->waiting_queue)
	{
		free(control->waiting_queue);
		control->waiting_queue = NULL;
	}
}

void	init_control(t_control *control)
{
	control->coders = NULL;
	control->dongles = NULL;
	control->waiting_queue = NULL;
	control->waiting_count = 0;
	control->next_ticket = 0;
}

int	main(int argc, char *arg[])
{
	t_control		control;
	int				created;

	init_control(&control);
	if (check_args(argc, arg, &control) == 1)
		return (1);
	if (init_waiting_queue(&control) == 1)
		return (1);
	control.start_time = current_time();
	if (init_coders(&control) == -1 || init_dongles(&control) == -1)
		return (free_control(&control), 1);
	dongles_assignation(&control);
	created = 0;
	if (init_sync(&control) != 0)
		return (free_control(&control), 1);
	created = 3;
	if (run_simulation(&control) != 0)
	{
		destroy_sync(&control, created);
		return (free_control(&control), 1);
	}
	destroy_sync(&control, created);
	return (free_control(&control), 0);
}
