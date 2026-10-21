/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janrodri <janrodri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/12 18:53:09 by janrodri          #+#    #+#             */
/*   Updated: 2026/10/21 15:18:05 by janrodri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

/*If the result is invalid, returns '-1'.
If the result is valid but is 0, return '-2',
If the result is valid but is over INT_MAX, return '-3',
Checks if a digit surpass INT_MAX = 2147483647*/
int	parse_int(char *arg)
{
	int	i;

	i = 0;
	if (arg == NULL || arg[0] == '\0')
		return (-4);
	if (arg[0] == '-')
		return (-1);
	while (i < (int) strlen(arg))
	{
		if ((arg[i] >= '0') && (arg[i] <= '9'))
			i++;
		else
			return (-1);
	}
	if (atoi(arg) == 0)
		return (-2);
	if (
		(strlen(arg) == 10 && strcmp("2147483647", arg) < 0)
		|| (strlen(arg) > 10))
		return (-3);
	return (atoi(arg));
}

int	print_error_args(void)
{
	printf("Error.  The expected 8 arguments are:\n"
		"<number_of_coders> <time_to_burnout> <time_to_compile>\n"
		"<time_to_debug> <time_to_refactor> <number_of_compiles_required>\n"
		"<dongle_cooldown> <scheduler>\n");
	return (1);
}

int	limit_cases(char *arg, int index_arg)
{
	int	parsed;

	parsed = parse_int(arg);
	if (index_arg == 8 && strcmp(arg, "fifo") != 0 && strcmp(arg, "edf") != 0)
	{
		printf("The <scheduler> can only be: \"edf\" or \"fifo\"\n");
		return (1);
	}
	else if (index_arg < 8 && parsed < 0)
	{
		if (parsed == -1)
			printf("The argument %s is not a positive int\n", arg);
		if (parsed == -2)
			printf("The given values can't be '0'\n");
		if (parsed == -3)
			printf("The given value: '%s' can't surpass 2147483647\n",
				arg);
		if (parsed == -4)
			printf("The given values can't be empty\n");
		return (1);
	}
	return (0);
}

void	control_args(t_control *control, char *args[])
{
	(*control).number_of_coders = atoi(args[1]);
	(*control).time_to_burnout = atoi(args[2]);
	(*control).time_to_compile = atoi(args[3]);
	(*control).time_to_debug = atoi(args[4]);
	(*control).time_to_refactor = atoi(args[5]);
	(*control).number_of_compiles_required = atoi(args[6]);
	(*control).dongle_cooldown = atoi(args[7]);
	(*control).scheduler = args[8];
	control->simulation_status = SIM_RUNNING;
}

int	check_args(int argc, char **arg, t_control *control)
{
	int	i;
	int	parsed;

	if (argc != 9)
		return (print_error_args());
	i = 1;
	while (i < 9)
	{
		parsed = limit_cases(arg[i], i);
		if (parsed == 1)
			return (1);
		i++;
	}
	control_args(control, arg);
	return (0);
}
