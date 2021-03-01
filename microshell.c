/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   microshell.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: youlee <youlee@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/03/01 17:04:19 by youlee            #+#    #+#             */
/*   Updated: 2021/03/01 18:24:23 by youlee           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define PIPEOUT 0
#define PIPEIN 1

#define STDIN 0
#define STDOUT 1
#define STDERR 2

#define TYPEEND 0
#define TYPEPIPE 1
#define TYPEBREAK 2

#ifdef TEST_SH
# define TEST	1
#else
# define TEST	0
#endif

typedef struct s_list
{
	char		**args;
	int			len;
	int			type;
	int			pipes[2];
	struct s_list	*next;
	struct s_list	*prev;
}				t_list;

int		ft_strlen(char const *str)
{
	int i;

	i = 0;
	while (str[i])
		i++;
	return (i);
}

int		show_error(char const *str)
{
	if (str)
		write(STDERR, str, ft_strlen(str));
	return (EXIT_FAILURE);
}

int		exit_fatal(void)
{
	show_error("error: fatal\n");
	exit(EXIT_FAILURE);
	return (EXIT_FAILURE);
}

void	*exit_fatal_ptr(void)
{
	exit_fatal();
	exit(EXIT_FAILURE);
	return (NULL);
}

char	*ft_strdup(char const *str)
{
	char *ret;
	int		i;

	if (!(ret = (char*)malloc(sizeof(*ret) * (ft_strlen(str) + 1))))
		return (exit_fatal_ptr());
	i = 0;
	while (str[i])
	{
		ret[i] = str[i];
		i++;
	}
	ret[i] = 0;
	return (ret);
}

int		add_arg(t_list *cmd, char *arg)
{
	char	**temp;
	int		i;

	i = 0;
	temp = NULL;
	if (!(temp = (char**)malloc(sizeof(*temp) * (cmd->len + 2))))
		return (exit_fatal());
	while (i < cmd->len)
	{
		temp[i] = cmd->args[i];
		i++;
	}
	if (cmd->len > 0)
		free(cmd->args);
	cmd->args = temp;
	cmd->args[i++] = ft_strdup(arg);
	cmd->args[i] = 0;
	cmd->len++;
	return (EXIT_SUCCESS);
}

int		list_push(t_list **list, char *arg)
{
	t_list *new;

	if (!(new = (t_list*)malloc(sizeof(*new))))
		return (exit_fatal());
	new->args = NULL;
	new->len = 0;
	new->type = TYPEEND;
	new->prev = NULL;
	new->next = NULL;
	if (*list)
	{
		(*list)->next = new;
		new->prev = *list;
	}
	*list = new;
	return (add_arg(new, arg));
}

int		list_back(t_list **list)
{
	while (*list && (*list)->prev)
		*list = (*list)->prev;
	return (EXIT_SUCCESS);
}

int		list_clear(t_list **cmds)
{
	t_list	*temp;
	int		i;

	list_back(cmds);
	while (*cmds)
	{
		temp = (*cmds)->next;
		i = 0;
		while (i < (*cmds)->len)
			free((*cmds)->args[i++]);
		free((*cmds)->args);
		free(*cmds);
		*cmds=temp;
	}
	*cmds=NULL;
	return (EXIT_SUCCESS);
}

int		parse_arg(t_list **list, char *arg)
{
	int		is_break;

	is_break = (strcmp(";", arg) == 0);
	if (is_break && !*list)
		return (EXIT_SUCCESS);
	else if (!is_break && (!*list || (*list)->type > TYPEEND))
		return (list_push(list, arg));
	else if (strcmp("|", arg) == 0)
		(*list)->type = TYPEPIPE;
	else if (is_break)
		(*list)->type = TYPEBREAK;
	else
		return (add_arg(*list, arg));
	return (EXIT_SUCCESS);
}

int	exec_cmd(t_list *cmd, char **env)
{
	pid_t pid;
	int		ret;
	int		status;
	int		pipe_open;

	ret = EXIT_FAILURE;
	pipe_open = 0;
	if (cmd->type == TYPEPIPE || (cmd->prev && cmd->prev->type == TYPEPIPE))
	{
		pipe_open = 1;
		if (pipe(cmd->pipes))
			return (exit_fatal());
	}
	pid = fork();
	if (pid < 0)
		return (exit_fatal());
	else if (pid == 0)
	{
		if (cmd->type == TYPEPIPE
				&& dup2(cmd->pipes[PIPEIN], STDOUT) < 0)
			return (exit_fatal());
		if (cmd->prev && cmd->prev->type == TYPEPIPE
				&& dup2(cmd->prev->pipes[PIPEOUT], STDIN) < 0)
			return (exit_fatal());
		if ((ret = execve(cmd->args[0], cmd->args, env)) < 0)
		{
			show_error("error: cannot execute ");
			show_error(cmd->args[0]);
			show_error("\n");
		}
		exit(ret);
	}
	else
	{
		waitpid(pid, &status, 0);
		if (pipe_open)
		{
			close(cmd->pipes[PIPEIN]);
			if (!cmd->next || cmd->type == TYPEBREAK)
				close(cmd->pipes[PIPEOUT]);
		}
		if (cmd->prev && cmd->prev->type == TYPEPIPE)
			close(cmd->prev->pipes[PIPEOUT]);
		if (WIFEXITED(status))
			ret = WEXITSTATUS(status);	
	}
	return (ret);
}
int	exec_cmds(t_list **list, char**env)
{
	t_list *crt;
	int		ret;

	ret = EXIT_SUCCESS;
	list_back(list);
	while (*list)
	{
		crt = *list;
		if (strcmp("cd", crt->args[0]) == 0)
		{
			ret = EXIT_SUCCESS;
			if (crt->len < 2)
				ret = show_error("error: cd: bad arguments\n");
			else if (chdir(crt->args[1]))
			{
				ret = show_error("error: cd: cannot change directory to ");
				show_error(crt->args[1]);
				show_error("\n");
			}
		}
		else
			ret = exec_cmd(crt, env);
		if (!(*list)->next)
			break ;
		*list = (*list)->next;
	}
	return (ret);
}
int main(int ac, char **av, char **ev)
{
	t_list	*cmds;
	int		i;
	int		ret;

	ret = EXIT_SUCCESS;
	i = 1;
	cmds = NULL;
	while (i < ac)
		parse_arg(&cmds, av[i++]);
	if (cmds)
		ret = exec_cmds(&cmds, ev);
	list_clear(&cmds);
	if (TEST)
		while (1);
	return (ret);
}
