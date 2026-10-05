/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/29 17:52:45 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/05 17:18:44 by janrodri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*If the result is invalid, returns '-1'*/
int	parse_int(char *arg)
{
	int	i;

	i = 0;
	if (arg[0] == '-')
		return (-1);
	while (i < (int) strlen(arg))
	{
		if ((arg[i] >= '0') && (arg[i] <= '9'))
			i++;
		else
			return (-1);
	}
	return (atoi(arg));
}

int	main(int argc, char *arg[])
{
	if (argc != 9)
	{
		printf("Not enough arguments, the expected arguments are: "
			"<number_of_coders> <time_to_burnout> <time_to_compile>\n"
			"<time_to_debug> <time_to_refactor> <number_of_compiles_required>"
			" <dongle_cooldown> <scheduler>\n"
			);
	}
	(void) arg;
	return (0);
}
