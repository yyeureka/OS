#include<stdio.h>
#include<stdlib.h>
#include<libvirt/libvirt.h>
#include<math.h>
#include<string.h>
#include<unistd.h>
#include<limits.h>
#include<signal.h>
#define MIN(a,b) ((a)<(b)?a:b)
#define MAX(a,b) ((a)>(b)?a:b)

int is_exit = 0; // DO NOT MODIFY THIS VARIABLE


void CPUScheduler(virConnectPtr conn,int interval);

/*
DO NOT CHANGE THE FOLLOWING FUNCTION
*/
void signal_callback_handler()
{
	printf("Caught Signal");
	is_exit = 1;
}

/*
DO NOT CHANGE THE FOLLOWING FUNCTION
*/
int main(int argc, char *argv[])
{
	virConnectPtr conn;

	if(argc != 2)
	{
		printf("Incorrect number of arguments\n");
		return 0;
	}

	// Gets the interval passes as a command line argument and sets it as the STATS_PERIOD for collection of balloon memory statistics of the domains
	int interval = atoi(argv[1]);
	
	conn = virConnectOpen("qemu:///system");
	if(conn == NULL)
	{
		fprintf(stderr, "Failed to open connection\n");
		return 1;
	}

	// Get the total number of pCpus in the host
	signal(SIGINT, signal_callback_handler);

	while(!is_exit)
	// Run the CpuScheduler function that checks the CPU Usage and sets the pin at an interval of "interval" seconds
	{
		CPUScheduler(conn, interval);
		sleep(interval);
	}

	// Closing the connection
	virConnectClose(conn);
	return 0;
}

/* COMPLETE THE IMPLEMENTATION */
void CPUScheduler(virConnectPtr conn, int interval)
{
	// pCpus time
	int nparams = 0;
	int cpuNum = VIR_NODE_CPU_STATS_ALL_CPUS;
	virNodeCPUStatsPtr params;
	if (virNodeGetCPUStats(conn, cpuNum, NULL, &nparams, 0) == 0 && nparams != 0) {
		if ((params = malloc(sizeof(virNodeCPUStats) * nparams)) == NULL)
			goto error;
		memset(params, 0, sizeof(virNodeCPUStats) * nparams);
		if (virNodeGetCPUStats(conn, cpuNum, params, &nparams, 0))
			goto error;
	}

	unsigned long long busy_time = 0;
	for (int i = 0; i < nparams; i++) {
		if (strcmp(params[i].field, VIR_NODE_CPU_STATS_USER) == 0 ||
		    strcmp(params[i].field, VIR_NODE_CPU_STATS_KERNEL) == 0) {
			busy_time += params[i].value;
		}
		//printf("pCPUs %s: %llu ns\n", params[i].field, params[i].value);
	}


	// get active domains list
	unsigned int flags = VIR_CONNECT_LIST_DOMAINS_ACTIVE | VIR_CONNECT_LIST_DOMAINS_RUNNING;
	virDomainPtr *domains;
	int num_domains;
	num_domains = virConnectListAllDomains(conn, &domains, flags);
	check(num_domains > 0, "Failed to list all domains\n");
	struct DomainsList *list = malloc(sizeof(struct DomainsList));
	list->count = num_domains;
	list->domains = domains;


	// get domain statistics
	unsigned int stats = 0;
	virDomainStatsRecordPtr *records = NULL;

	stats = VIR_DOMAIN_STATS_VCPU;
	check(virDomainListGetStats(list.domains, stats,
				    &records, 0) > 0,
	      "Could not get domains stats");
}




