/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongles.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/14 17:02:56 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/14 17:18:09 by janrodri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	init_dongles(t_control *control)
{
	control->dongles = malloc(sizeof(t_dongle) * control->number_of_coders);
	if (!control->dongles)
		return (-1);
	return (0);
}
