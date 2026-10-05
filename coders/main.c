/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/29 17:52:45 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/05 18:58:08 by janrodri         ###   ########.fr       */
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

int	print_error_args(void)
{
	printf("Error.\nThe expected arguments are:\n"
		"<number_of_coders> <time_to_burnout> <time_to_compile>\n"
		"<time_to_debug> <time_to_refactor> <number_of_compiles>\n"
		"<dongle_cooldown> <scheduler>\n");
	return (1);
}

int	check_args(int argc, char **arg)
{
	int	i;

	if (argc != 9)
		return (print_error_args());
	i = 1;
	while (i < 9)
	{
		if (i == 8 && strcmp(arg[8], "fifo") != 0 && strcmp(arg[8], "edf") != 0)
		{
			printf("The <scheduler> can only be: \"edf\" or \"fifo\"\n");
			return (1);
		}
		else if (i < 8 && parse_int(arg[i]) == -1)
		{
			printf("The argument %s is not a positive int\n", arg[i]);
			return (1);
		}
		i++;
	}
	return (0);
}

int	main(int argc, char *arg[])
{
	if (check_args(argc, arg) == 1)
		return (1);
	return (0);
}
