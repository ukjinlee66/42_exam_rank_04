#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define		PIPE_OUT	0
#define		PIPE_IN		1

#define		STD_IN		0
#define		STD_OUT		1
#define		STD_ERR		2

#define		END			0
#define		PIPE		1
#define		BREAK		2

#ifndef TEST_SH
#define TEST 1
#else
#define TEST 0
#endif

typedef struct	s_micro
{
	char			**str;
	int				len;
	int				type;
	int				pipes[2];
	struct s_micro	*next;
	struct s_micro	*prev;
}				t_micro;

int		ft_strlen(char *str)
{
	int si;

	si = 0;
	while (str[si])
		si++;
	return (si);
}
//error function
int		show_error(char *str)
{
	if (str)
		write(STD_ERR, str, ft_strlen(str));
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

//string function


char	*ft_strdup(char *str)
{
	char	*ret;
	int		i;

	if(!(ret = (char*)malloc(sizeof(*ret) * (ft_strlen(str) + 1))))
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



//list function
int		add_str(t_micro *new, char *str)
{
	char	**temp;
	int		i;

	i = 0;
	temp = NULL;
	if (!(temp = (char**)malloc(sizeof(*temp) * (new->len + 2))))
		return (exit_fatal());
	while (i < new->len)
	{
		temp[i] = new->str[i];
		i++;
	}
	if (new->len > 0)
		free(new->str);
	new->str = temp;
	new->str[i++] = ft_strdup(str);
	new->str[i] = 0;
	new->len++;
	return (EXIT_SUCCESS);
}

int		list_push(t_micro **list, char *str)
{
	t_micro *new;
	
	if (!(new = (t_micro*)malloc(sizeof(*new))))
		return (exit_fatal());
	new->str = NULL;
	new->len = 0;
	new->next = NULL;
	new->prev = NULL;
	new->type = END;
	
	if (*list)
	{
		(*list)->next = new;
		new->prev = *list;
	}
	*list = new;
	return (add_str(new, str));
}

int		list_back(t_micro **list)
{
	while (*list && (*list)->prev)
		*list = (*list)->prev;
	return (EXIT_SUCCESS);
}

int		list_clear(t_micro **list)
{
	t_micro *temp;
	int		i;

	list_back(list);
	while (*list)
	{
		temp = (*list)->next;
		i = 0;
		while (i < (*list)->len)
			free((*list)->str[i++]);
		free((*list)->str);
		free(*list);
		*list = temp;
	}
	*list = NULL;
	return (EXIT_SUCCESS);
}

//exec function
int		parse_arg(t_micro **list, char *av)
{
	int	is_break;

	is_break = (strcmp(";", av) == 0);
	if (is_break && !*list)
		return (EXIT_SUCCESS);
	else if (!is_break && (!*list || (*list)->type > END))
		return (list_push(list, av));
	else if (strcmp("|", av) == 0)
		(*list)->type = PIPE;
	else if (is_break)
		(*list)->type = BREAK;
	else
		return (add_str(*list, av));
	return (EXIT_SUCCESS);
}

int		exec_cmd(t_micro *list, char **ev)
{
	pid_t	pid;
	int		ret;
	int		status;
	int		pipe_open;

	ret = EXIT_FAILURE;
	pipe_open = 0;
	if (list->type == PIPE || (list->prev && list->prev->type == PIPE))
	{
		pipe_open = 1;
		if (pipe(list->pipes))
			return (exit_fatal());
	}
	pid = fork();
	if (pid < 0)
		return (exit_fatal());
	else if (pid == 0)
	{
		if (list->type == PIPE 
				&& dup2(list->pipes[PIPE_IN], STD_OUT) < 0)
			return (exit_fatal());
		if (list->prev && list->prev->type == PIPE 
				&& dup2(list->prev->pipes[PIPE_OUT], STD_IN) < 0)
			return (exit_fatal());
		if ((ret = execve(list->str[0], list->str, ev)) < 0)
		{
			show_error("error: cannot execute ");
			show_error(list->str[0]);
			show_error("\n");
		}
		exit(ret);
	}
	else
	{
		waitpid(pid, &status, 0);
		if (pipe_open)
		{
			close(list->pipes[PIPE_IN]);
			if (!list->next || list->type == BREAK)
				close(list->pipes[PIPE_OUT]);
		}
		if (list->prev && list->prev->type == PIPE)
			close(list->prev->pipes[PIPE_OUT]);
		if (WIFEXITED(status))
			ret = WEXITSTATUS(status);
	}
	return (ret);
}

int		exec_cmds(t_micro **list, char **ev)
{
	t_micro	*crt;
	int		ret;

	ret = EXIT_SUCCESS;
	list_back(list);
	while (*list)
	{
		crt = *list;
		if (strcmp("cd", crt->str[0]) == 0)
		{
			ret = EXIT_SUCCESS;
			if (crt->len < 2)
				ret = show_error("error: cd: bad arguments\n");
			else if (chdir(crt->str[1]))
			{
				ret = show_error("error: cd: cannot change directory to ");
				show_error(crt->str[1]);
				show_error("\n");
			}
		}
		else
			ret = exec_cmd(crt, ev);
		if (!(*list)->next)
			break ;
		*list = (*list)->next;
	}
	return (ret);
}


int main(int ac, char **av, char **ev)
{
	t_micro *list;
	int		i;
	int		ret;
	
	ret = EXIT_SUCCESS;
	list = NULL;
	i = 1;
	while (i < ac)
		parse_arg(&list, av[i++]);
	if (list)
		ret = exec_cmds(&list, ev);
	list_clear(&list);
	//if (TEST)
	//	while (1);
	return (ret);	
}
