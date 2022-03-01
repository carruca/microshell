#include "unistd.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"

#define STDERR		2

size_t	ft_strlen(char *str)
{
	size_t	len;

	len = 0;
	while (str[len])
		len++;
	return len;
}

void	error_print(char *msg, char *arg)
{
	size_t	msg_len;
	size_t	arg_len;

	msg_len = ft_strlen(msg);
	write(STDERR, msg, msg_len);
	if (arg)
	{
		arg_len = ft_strlen(arg);
		write(STDERR, arg, arg_len);
	}
	write(STDERR, "\n", 1);
}

char	**get_next(char **argv, char *end)
{
	while (argv && *argv)
	{
		if (!strcmp(*argv, end))
		{
			*argv = NULL;
			return ++argv;
		}
		argv++;
	}
	return NULL;
}

void	builtin_cd(char **argv)
{
	int	ret;

	if (!argv || *(argv + 1))
		error_print("error: cd: wrong arguments", NULL);
	ret = chdir(*argv);
	if (ret == -1)
		error_print("error: cd: cannot change directory to ", *argv);
}

void	execute_cmd(char **argv, char **env, int *fd_pipe, int fd_in)
{
	pid_t	child_pid;

	child_pid = fork();
	if (child_pid == -1)
	{
		error_print("error: fatal", NULL);
		exit(EXIT_FAILURE);
	}
	else if (child_pid == 0)
	{
		if (fd_in)
		{
			if (dup2(fd_in, 0) == -1)
			{
				error_print("error: fatal", NULL);
				exit(EXIT_FAILURE);
			}
		}
		if (fd_pipe[1] != 1)
		{
			if (dup2(fd_pipe[1], 1) == -1)
			{
				error_print("error: fatal", NULL);
				exit(EXIT_FAILURE);
			}
		}
		if (execve(*argv, argv, env) == -1)
			error_print("error: cannot execute ", *argv);
		exit(EXIT_SUCCESS);
	}
	waitpid(child_pid, NULL, 0);
	return ;
}

void	execute_pipeline(char **argv, char **env)
{
	char	**next_cmd;
	int		fd_pipe[2];
	int		fd_in;

	fd_pipe[0] = 0;
	fd_in = 0;
	while (argv && *argv)
	{
		fd_pipe[1] = 1;
		next_cmd = get_next(argv, "|");
		if (!strcmp(*argv, "cd"))
			builtin_cd(++argv);
		else
		{
			if (next_cmd)
			{
				if (pipe(fd_pipe) == -1)
				{
					error_print("error: fatal", NULL);
					exit(EXIT_FAILURE);
				}
			}
			execute_cmd(argv, env, fd_pipe, fd_in);
			if (fd_in)
			{
				if (close(fd_in) == -1)
				{
					error_print("error: fatal", NULL);
					exit(EXIT_FAILURE);
				}
			}
			if (fd_pipe[1] != 1)
			{
				if (close(fd_pipe[1]) == -1)
				{
					error_print("error: fatal", NULL);
					exit(EXIT_FAILURE);
				}
			}
			if (fd_pipe[0] != 0)
				fd_in = fd_pipe[0];
		}
		argv = next_cmd;
	}
	return ;
}

int	main(int argc, char **argv, char **env)
{
	char	**next_pipeline;

	argv++;
	while (argc && argv && *argv)
	{
		next_pipeline = get_next(argv, ";");
		execute_pipeline(argv, env);
		argv = next_pipeline;
	}
	return 0;
}
