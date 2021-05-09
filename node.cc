#include "Node.hh"

#ifdef READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

/*
struct {
  const char *command;
  void (*handler)(const char *);
} cmd_table[] = {
  {"help", help_cmd},
  {"h", help_cmd},
  {"interfaces", interfaces_cmd},
  {"i", interfaces_cmd},
  {"routes", routes_cmd},
  {"r", routes_cmd},
  {"down", down_cmd},
  {"up", up_cmd},
  {"send", send_cmd}
};
*/

timer_t rip_update_timer;
timer_t rip_refresh_timer;
MessageQueue<int> Node::_timerQueue;

static void timerHandler( int sig, siginfo_t *si, void *uc )
{

    timer_t *tidp = (timer_t *)(si->si_value.sival_ptr);

    if ( *tidp == rip_update_timer ) {
        //fprintf(stdout,"update timer\n");
	Node::_timerQueue.push(UPDATE_TIMER_ID);
    }
    else if ( *tidp == rip_refresh_timer ){
    	//fprintf(stdout, "refresh timer\n");
	Node::_timerQueue.push(REFRESH_TIMER_ID);
    }
    else
        fprintf(stdout,"invalid timer for %d, context:%p\n", sig, uc); 
}


static int create_timer(timer_t *timer, int expire_sec, int interval_sec) {
    struct sigevent         te;
    struct itimerspec       its;
    struct sigaction        sa;
    int                     sigNo = SIGRTMIN;

    /* Set up signal handler. */
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = timerHandler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(sigNo, &sa, NULL) == -1)
    {
        fprintf(stderr, " Failed to setup signal handling.\n");
        return(-1);
    }

    /* Set and enable alarm */
    te.sigev_notify = SIGEV_SIGNAL;
    te.sigev_signo = sigNo;
    te.sigev_value.sival_ptr = timer;
    timer_create(CLOCK_REALTIME, &te, timer);

    its.it_interval.tv_sec = interval_sec;
    its.it_interval.tv_nsec = 0;
    its.it_value.tv_sec = expire_sec;
    its.it_value.tv_nsec = 0;
    timer_settime(*timer, 0, &its, NULL);

    return(0);

}

void print_ip(unsigned int ip)
{
    unsigned char bytes[4];
    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;   
    fprintf(stdout, "%d.%d.%d.%d\n", bytes[3], bytes[2], bytes[1], bytes[0]);        
}


int main(int argc, char **argv){
    if (argc != 2) {
	dbg(DBG_ERROR, "usage: %s <linkfile>\n", argv[0]);
	return -1;
    }

#ifdef READLINE
    char* line;
    rl_bind_key('\t', rl_complete);
#else
    char line[LINE_MAX];
#endif
    char cmd[LINE_MAX];
    uint8_t append_hist;
    int ret;

    Node *node = new Node(argv[1]);
  
    create_timer(&rip_update_timer, UPDATE_TIMER_VAL, UPDATE_TIMER_VAL);
    create_timer(&rip_refresh_timer, REFRESH_TIMER_VAL, REFRESH_TIMER_VAL);

    while (1) {
#ifdef READLINE
	if (!(line = readline("> "))) break;
#else
	dbg(DBG_ERROR, "> "); (void)fflush(stdout);
	if (!fgets(line, sizeof(line), stdin)) break;
	if (strlen(line) > 0 && line[strlen(line)-1] == '\n')
	    line[strlen(line)-1] = 0;
#endif

	ret = sscanf(line, "%s", cmd);
	if (ret != 1) {
	    if (ret != EOF) node->help_cmd(line);
	    continue;
	}
	if (!strcmp(cmd, "q")) break;

	node->execute_cmd(cmd, line, &append_hist);
	/*
	for (i=0; i < sizeof(cmd_table) / sizeof(cmd_table[0]); i++){
	    node->execute_cmd(cmd, line);
		if (!strcmp(cmd, cmd_table[i].command)){
		cmd_table[i].handler(line);
		break;
	    }
	}
        
	if (i == sizeof(cmd_table) / sizeof(cmd_table[0])){
	    dbg(DBG_ERROR, "error: no valid command specified\n");
	    node->help_cmd(line);
	    continue;
	}
        */
#ifdef READLINE
	if (append_hist) add_history(line);
	free(line);
#endif
    }


    //TODO Clean up your layers!


    printf("\nGoodbye!\n\n");
    return 0;
}


