/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coders.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/14 16:24:55 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/14 17:24:51 by janrodri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	init_coders(t_control *control)
{
	control->coders = malloc(sizeof(t_coder) * control->number_of_coders);
	if (!control->coders)
		return (-1);
	return (0);
}
