/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   compiling.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/11 20:39:17 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/12 19:24:23 by janrodri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	compile_previous(t_coder coder)
{
	if (coder.left_dongle && coder.right_dongle)
		return (printf("Can compile"), 1);
	else
		return (printf("Can't compile"), 0);
}

void	compiling(t_control control, t_dongle *left,
	t_dongle *right, t_coder coder)
{
	(*left).state_dongle = 0;
	(*right).state_dongle = 0;
	(void) control;
	(void) coder;
}
