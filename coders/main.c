/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/29 17:52:45 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/14 13:33:39 by janrodri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	main(int argc, char *arg[])
{
	t_control	control;

	if (check_args(argc, arg, &control) == 1)
		return (1);
	return (0);
}
