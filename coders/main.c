/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/29 17:52:45 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/17 20:09:00 by janrodri         ###   ########.fr       */
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
}

int	main(int argc, char *arg[])
{
	t_control		control;
	int				created;

	control.coders = NULL;
	control.dongles = NULL;
	if (check_args(argc, arg, &control) == 1)
		return (1);
	if (init_coders(&control) == -1 || init_dongles(&control) == -1)
		return (free_control(&control), 1);
	dongles_assignation(&control);
	control.start_time = current_time();
	created = 0;
	if (init_sync(&control) != 0)
		return (free_control(&control), 1);
	created = 3;
	log_line(&control, "simulation started", 0);
	destroy_sync(&control, created);
	return (free_control(&control), 0);
}
