/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongles.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/14 17:02:56 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/15 14:04:57 by janrodri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	dongles_args(t_dongle *dongle, int i)
{
	dongle->state_dongle = 1;
	dongle->dongle_id = i;
}

int	init_dongles(t_control *control)
{
	int	i;

	control->dongles = malloc(sizeof(t_dongle) * control->number_of_coders);
	if (!control->dongles)
	{
		free(control->dongles);
		control->dongles = NULL;
		return (-1);
	}
	i = 0;
	while (i < control->number_of_coders)
	{
		dongles_args(&(control->dongles[i]), i + 1);
		i++;
	}
	return (0);
}

void	dongles_assignation(t_control *control)
{
	int		n;
	int		i;
	t_coder	*coders;

	n = control->number_of_coders;
	coders = control->coders;
	i = 0;
	while (i < n)
	{
		if (i == 0)
		{
			coders[0].left_dongle = i + 1;
			coders[0].right_dongle = n;
		}
		else
		{
			coders[i].left_dongle = i + 1;
			coders[i].right_dongle = i ;
		}
		i++;
	}
}
