/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coders.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/14 16:24:55 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/14 20:10:22 by janrodri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	coder_args(t_coder *coder, int i, t_control *control)
{
	coder->id = i;
	coder->number_of_compiles_done = 0;
	coder->last_compilation = 0;
	coder->control = control;
	coder->left_dongle = 0;
	coder->right_dongle = 0;
}

int	init_coders(t_control *control)
{
	int	i;

	control->coders = malloc(sizeof(t_coder) * control->number_of_coders);
	if (!control->coders)
	{
		free(control->coders);
		control->coders = NULL;
		return (-1);
	}
	i = 0;
	while (i < control->number_of_coders)
	{
		coder_args(&(control->coders[i]), i + 1, control);
		i++;
	}
	return (0);
}
