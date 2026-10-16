/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   resources.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/15 16:57:14 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/16 20:06:54 by janrodri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	dongles_available(t_control *control, int left_id, int right_id)
{
	if ((control->dongles[left_id - 1].state_dongle == DONGLE_FREE) && (
			control->dongles[left_id - 1].available_at))
		if ((control->dongles[right_id - 1].state_dongle == DONGLE_FREE) && (
				control->dongles[right_id - 1].available_at))
			return (1);
	return (0);
}
