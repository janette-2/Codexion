/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test_header.h                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/09 19:16:08 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/09 19:51:12 by janrodri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef TEST_HEADER_H
# define TEST_HEADER_H

# include <pthread.h>

typedef struct s_coder
{
	int	id;
	int	number_of_compiles_required;
	int	time_to_burnout;
	int	time_to_compile;
	int	time_to_debug;
	int	time_to_refactor;

}	t_coder;

typedef struct s_dongle
{
	int	state_boolean;
	int	dongle_cooldown;
}	t_dongle;

typedef struct s_control
{
	int	number_of_coders;
	int	scheduler;

}	t_control;

#endif