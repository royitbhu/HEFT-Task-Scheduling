/*******************************************************
	    HEFT DUPLICATION IMPLEMENTATION
********************************************************/
//NOTES
//UPPER RANK ARRAY(tasks_upper_rank[]) INITIALIZED TO -1 SO AS TO PREVENT REDUNDANT CALCULATION OF UPPER RANKS
//COMMUNICATION COST MATRIX(data[][]) HAS -1 VALUE FOR NON COMMUNICATING TASKS


#include<stdio.h>
#include<stdlib.h>
int no_tasks,no_machines; 
double **computation_costs,**data_transfer_rate,**data,*tasks_upper_rank;
int *sorted_tasks,*worst_est,**duplication_status;
struct TaskProcessor
{
       double AST;
       double AFT;
       int heft;
};

struct TaskProcessor **schedule;
/*******************************************************
	FUNCTION FOR DETERMINATION OF UPPER RANK	
********************************************************/
//CALCULATE AVG COMMUNCATION COST AND GIVE FEED TO SORTED TASK ARRAY
void insertinto(int task,double rank)
{
     static int pos;
     int i;
     for(i=pos-1;i>=0;i--)
        if(rank>tasks_upper_rank[sorted_tasks[i]]) 
            sorted_tasks[i+1]=sorted_tasks[i];
        else
            break;
     sorted_tasks[i+1]=task;
     pos++;
}
double avg_communicationcost(int source,int destination)
{
       int i,j;
       double avg=0.0;
       for(i=0;i<no_machines;i++)
          for(j=0;j<no_machines;j++)
          {
             if(data_transfer_rate[i][j]!=0)
             	avg+=(data[source][destination]/data_transfer_rate[i][j]);
          }
       avg=avg/(no_machines*no_machines-no_machines);
       return avg;
}

double calculate_upper_rank(int task)
{
	int j;
	double avg_communication_cost,successor,avg=0.0,max=0,rank_successor;
    for(j=0;j<no_machines;j++)
         avg+=computation_costs[task][j];
    avg/=no_machines;
    for(j=0;j<no_tasks;j++)
    {
        if(data[task][j]!=-1)             //check if a successor
        {
           avg_communication_cost=avg_communicationcost(task,j);
           if(tasks_upper_rank[j]==-1)
           {
              rank_successor= tasks_upper_rank[j]= calculate_upper_rank(j);
              insertinto(j,rank_successor);
           }
           else
               rank_successor= tasks_upper_rank[j];     
           successor=avg_communication_cost+rank_successor;
           if(max<successor)
              max=successor;
        }
     }
     return(avg+max);
}

/*******************************************************
	FUNCTION FOR DETERMINATION OF SCHEDULE	 
********************************************************/
void insertslots(double **machineFreeTime,int current_pos, double start,double end)
{
	int i;
	//printf("%lf %lf\n",start,end);
	if(start < 0)
		start=0;
	for(i=current_pos-1;i>=0;i--)
	{
		if(start < machineFreeTime[i][0])
		{
			machineFreeTime[i+1][0]=machineFreeTime[i][0];
			machineFreeTime[i+1][1]=machineFreeTime[i][1];
		}
		else
			break;
	}
	machineFreeTime[i+1][0]=start;
	machineFreeTime[i+1][1]=end;
}
void findfreeslots(int processor,double **machineFreeTime,int *noslots)
{
       int i,j;
       *noslots=0;
       double highest_AFT=-99999.0,min=99999.0,smallest_AST=99999.0;
       for(i=0;i<no_tasks;i++)
       {
       		if(schedule[processor][i].AST==-1)
       			continue;
       		min=99999.0;
       		if(schedule[processor][i].AFT>highest_AFT)
           		highest_AFT=schedule[processor][i].AFT;
           	if(schedule[processor][i].AST<smallest_AST)
           		smallest_AST=schedule[processor][i].AST;
           	for(j=0;j<no_tasks;j++)
           	{
           		if(i==j)
           			continue;
           		if((schedule[processor][j].AST>=schedule[processor][i].AFT) && (schedule[processor][j].AST<min))
               		{
                       		min=schedule[processor][j].AST;
               		}
           	}
           	if(min<99998.0)
           	{
               		insertslots(machineFreeTime,*noslots,schedule[processor][i].AFT,min);
               		(*noslots)++;
           	}
       }
       insertslots(machineFreeTime,*noslots,highest_AFT,99999.0);
       (*noslots)++;
       if(smallest_AST>0)
       {
       		insertslots(machineFreeTime,*noslots,0.0,smallest_AST);
       		(*noslots)++;
       }
}
int isEntryTask(int task)
{
	int i;
	for(i=0;i<no_tasks;i++)
	{
		if(data[i][task]!=-1)
			return 0;
	}
	return 1;
}
int* findProcessor(int task,int *no_procs_task)
{
	*no_procs_task=0;
	int *procs_array=(int*)calloc(no_machines,sizeof(int));
	int i,j;
	for(i=0;i<no_machines;i++)
	{
		if(schedule[i][task].AST!=-1)
		{
			procs_array[(*no_procs_task)]=i;
			(*no_procs_task)++;
		}
	}
	return procs_array;
}
int worst_pred_processor=-1;
double find_EST(int task,int processor)
{
	int i,j,*procs_array, no_procs_task;
	double ST,EST=-99999.0,comm_cost,min=99999.99;
	for(i=0;i<no_tasks;i++)
	{
		min =99999.0;
		if(data[i][task]!=-1)
		{
			procs_array=findProcessor(i,&no_procs_task);
			//if(task==2) 
			//	printf("\npred_procs : %d\n",no_procs_task);
			for(j=0;j<no_procs_task;j++)
			{
				if(data_transfer_rate[procs_array[j]][processor]==0)
					comm_cost=0;
				else
					comm_cost=data[i][task]/data_transfer_rate[procs_array[j]][processor];
				ST=schedule[procs_array[j]][i].AFT + comm_cost;
				if(min>ST)
					min=ST;
			}
			if(EST<min)
			{
				EST=min;
				worst_pred_processor = i; 
			}
		}
	}
	//printf("EST for task :%d on processor: %d = %lf\n",task,processor,EST);
	return EST;
}
void calculate_EST_EFT(int task,int processor,struct TaskProcessor *EST_EFT)
{
	double **machineFreeTime,EST;
	int i;
	machineFreeTime=(double**)calloc(100,sizeof(double*));
     	for(i=0;i<100;i++)
        	 machineFreeTime[i]=(double*)calloc(2,sizeof(double));
        int noslots=0;
        findfreeslots(processor,machineFreeTime,&noslots);
        EST=find_EST(task,processor);
        EST_EFT->AST=EST;
        for(i=0;i<noslots;i++)
        {
        	if(EST<machineFreeTime[i][0])
        	{
        		if((machineFreeTime[i][0] + computation_costs[task][processor]) <= machineFreeTime[i][1])
        		{
        			EST_EFT->AST=machineFreeTime[i][0];
        			EST_EFT->AFT=machineFreeTime[i][0] + computation_costs[task][processor];
        			return;
        		}
        	}
        	if(EST>=machineFreeTime[i][0])
        	{
        		if((EST + computation_costs[task][processor]) <= machineFreeTime[i][1])
        		{
        			EST_EFT->AFT=EST_EFT->AST + computation_costs[task][processor];
        			return;
        		}
        	}
        }
}
void nullify_other_procs(int processor,int task)
{
	int i,j;
	for(i=0;i<no_machines;i++)
	{
		if(i==processor)
			continue;
		schedule[i][task].AST=-1;
		schedule[i][task].AFT=-1;
	}		
}
void make_schedule()
{
     int i,j,k,t=0,processor,task;
     double minCost=9999.99,min_EFT=9999.99;
     struct TaskProcessor *EST_EFT;
     EST_EFT=(struct TaskProcessor *)calloc(1,sizeof(struct TaskProcessor));
     EST_EFT->heft=1;
     EST_EFT->AST=-1;
     EST_EFT->AFT=-1;
     for(i=0;i<no_tasks;i++)
     {
        min_EFT=9999.99;
        task=sorted_tasks[i];
        if(isEntryTask(task))
        {
            for(j=0;j<no_machines;j++)
            {
                if(minCost>computation_costs[task][j])
                {
                     minCost=computation_costs[task][j];
                     processor=j;
                }
            }
            schedule[processor][task].AST=0;
            schedule[processor][task].AFT=minCost;
        }
        else
        {
            for(j=0;j<no_machines;j++)
            {
            	calculate_EST_EFT(task,j,EST_EFT);
            	//printf("%lf %lf %d\n",EST_EFT->AST,EST_EFT->AFT,EST_EFT->processor);
            	if(min_EFT>(EST_EFT->AFT))
            	{
            		schedule[j][task]=*EST_EFT;
            		processor=j;
            		min_EFT=EST_EFT->AFT;
            		worst_est[task]=worst_pred_processor;
            	}     
            }
            nullify_other_procs(processor,task);
        }
        /// by alok
        /*for(j=0;j<no_machines;j++)
        {
        	if(schedule[j][task].AST!=-1)
        	{
        		printf("Task Scheduled : %d\nProcessor :%d %lf %lf\n",task,j,schedule[j][i].AST,schedule[j][i].AFT);
        	}
        }*/
     }
}

/*******************************************************
	FUNCTION FOR DISPLAYING SCHEDULE	 
********************************************************/
void display_schedule()
{
	int i,j;
	printf("\n\nSCHEDULE\n\n");
	for(i=0;i<no_tasks;i++)
	{
		for(j=0;j<no_machines;j++)
		{
			if(schedule[j][i].AST!=-1)
			{
				if(schedule[j][i].heft==0)
					printf("DUPLICATED ENTRY  ");
				printf("TASK :%d PROCESSOR :%d AST :%lf AFT :%lf\n",i+1,j+1,schedule[j][i].AST,schedule[j][i].AFT);
			}
		}
		//duplication_status[i][schedule[j][i].processor] = 1;
	}
}
int orig_processor(int task)
{
	int i;
	for(i=0;i<no_machines;i++)
	{
		if((schedule[i][task].AST!=-1)&&(schedule[i][task].heft==1))
			return i;
	}
}
int scheduled(int task,int processor)
{
	if(schedule[processor][task].AST!=-1)
		return 1;
	else
		return 0;
}
void duplicate()
{
	int i,j,k,processor,pred_processor;
	double duration;
	double **machineFreeTime,EST;
	struct TaskProcessor *EST_EFT=(struct TaskProcessor *)calloc(1,sizeof(struct TaskProcessor));
	machineFreeTime=(double**)calloc(no_tasks+1,sizeof(double*));
     	for(i=0;i<(no_tasks+1);i++)
        	 machineFreeTime[i]=(double*)calloc(2,sizeof(double));
        int noslots=0;
	for(i=0;i<no_tasks;i++)
    	{
    		pred_processor=orig_processor(i);
    		for(j=0;j<no_tasks;j++)
    		{
    			if(j!=i && worst_est[j]==i)
    			{
    				processor=orig_processor(j);
    				EST=find_EST(i,processor);
    				if((processor==pred_processor)||((EST+computation_costs[i][processor])>schedule[processor][j].AST))
    					continue;
    				if(schedule[processor][i].heft==0)
    				{
    					schedule[processor][j].AST=-1;
    					schedule[processor][j].AFT=-1;
    					schedule[processor][j].heft=1;
    					calculate_EST_EFT(j,processor,EST_EFT);
    					schedule[processor][j]=*EST_EFT;
    					schedule[processor][j].heft=2;
    				}
    				if(scheduled(i,processor))
    					continue;
    				printf("%d %d\n",i+1,j+1);
    				calculate_EST_EFT(i,processor,EST_EFT);
    				printf("%d %lf %lf\n",i+1,EST_EFT->AST,EST_EFT->AFT);
    				if(EST_EFT->AFT < schedule[processor][j].AST)
    				{
    					schedule[processor][i]=*EST_EFT;
    					schedule[processor][i].heft=0;
    					schedule[processor][j].AST=-1;
    					schedule[processor][j].AFT=-1;
    					schedule[processor][j].heft=1;
    					calculate_EST_EFT(j,processor,EST_EFT);
    					printf("No duplication %d %lf %lf\n",j+1,EST_EFT->AST,EST_EFT->AFT);
    					schedule[processor][j]=*EST_EFT;
    					schedule[processor][j].heft=2;
    				}    				
    			}
    		}
    	}
}
int toBeScheduled(int task)
{
	int i;
	for(i=0;i<no_machines;i++)
	{
		if(schedule[i][task].heft==2 || schedule[i][task].heft==0)
			return 1;
	}
	return 0;
}
void reschedule()
{
	int i,j,processor,task,min_EFT,proc;
	struct TaskProcessor *EST_EFT=(struct TaskProcessor *)calloc(1,sizeof(struct TaskProcessor));
	for(i=0;i<no_tasks;i++)
	{
		min_EFT=99999.0;
		task=sorted_tasks[i];
		if(!toBeScheduled(task))
		{
			proc=orig_processor(task);
			schedule[proc][task].AST=-1;
			schedule[proc][task].AFT=-1;
			for(j=0;j<no_machines;j++)
            		{
		            	calculate_EST_EFT(task,j,EST_EFT);
            			//printf("%lf %lf %d\n",EST_EFT->AST,EST_EFT->AFT,EST_EFT->processor);
            			if(min_EFT>(EST_EFT->AFT))
            			{
            				schedule[j][task]=*EST_EFT;
            				schedule[j][task].heft=2;
            				processor=j;
            				min_EFT=EST_EFT->AFT;
            			}     
            		}
            		nullify_other_procs(processor,task);
		}
	}
}
int main()
{
	
	int i,j;
	FILE *fp;
	fp=fopen("Input1.txt","r+");
	//printf("Enter the number of tasks and machines\n");
	fscanf(fp,"%d%d",&no_tasks,&no_machines);
	//Initialize Arrays
	computation_costs=(double**)calloc(no_tasks,sizeof(double*));
	for(i=0;i<no_tasks;i++)
        	computation_costs[i]=(double*)calloc(no_machines,sizeof(double));
	data_transfer_rate=(double**)calloc(no_machines,sizeof(double*));
	for(i=0;i<no_machines;i++)
    		data_transfer_rate[i]=(double*)calloc(no_machines,sizeof(double));
	data=(double**)calloc(no_tasks,sizeof(double*));
	for(i=0;i<no_tasks;i++)
    		data[i]=(double*)calloc(no_tasks,sizeof(double));
	tasks_upper_rank=(double *)calloc(no_tasks,sizeof(double));
	for(i=0;i<no_tasks;i++)
        	tasks_upper_rank[i]=-1;
    	sorted_tasks=(int *)calloc(no_tasks,sizeof(int));
    	//alok
    	worst_est = (int*)calloc(no_tasks,sizeof(int));
    	duplication_status=(int**)calloc(no_tasks,sizeof(int*));
    	for(i=0;i<no_tasks;i++)
    		duplication_status[i]=(int*)calloc(no_machines,sizeof(int));
    	
    	schedule=(struct TaskProcessor**)calloc(no_machines,sizeof(struct TaskProcessor*));
    	for(i=0;i<no_machines;i++)
    	{
    	    	schedule[i]=(struct TaskProcessor*)calloc(no_tasks,sizeof(struct TaskProcessor));
    	    	for(j=0;j<no_tasks;j++)
    	    	{
    	    		schedule[i][j].AST=-1;
    	    		schedule[i][j].AFT=-1;
    	    		schedule[i][j].heft=1;
    	    	}
    	}
    	
	//printf("Enter the computation costs of each task\n");
	for(i=0;i<no_tasks;i++)
		for(j=0;j<no_machines;j++)
			fscanf(fp,"%lf",&computation_costs[i][j]);
	//printf("Enter the matrix of data transfer rate between two processors\n");
	for(i=0;i<no_machines;i++)
		for(j=0;j<no_machines;j++)
			fscanf(fp,"%lf",&data_transfer_rate[i][j]);
	//printf("Enter the matrix of data to be transferred between two tasks\n");
	for(i=0;i<no_tasks;i++)
	for(j=0;j<no_tasks;j++)
		fscanf(fp,"%lf",&data[i][j]);
	//calculate upper rank
	for(i=0;i<no_tasks;i++)
	{
    		if(tasks_upper_rank[i]==-1)
    		{
            		tasks_upper_rank[i]=calculate_upper_rank(i);
            		insertinto(i,tasks_upper_rank[i]);
        	}
	}
    	printf("UPPER RANKS OF TASKS :\n\n");
	for(i=0;i<no_tasks;i++)
        	printf("TASK NO. %d: %.2lf\n",i,tasks_upper_rank[i]);
    	for(i=0;i<no_tasks;i++)
        	printf("TASK NO. : %d\n",sorted_tasks[i]);
    	make_schedule();
    	display_schedule();
    	printf("Applying Duplication\n");
    	double duration,start,end,slot_start,slot_end;
    	int k,flag,l,k1;
    	for(i=0;i<no_tasks;i++)
    	{
    		printf("TASK :%d %d\n",i+1,worst_est[i]+1);
    	}
    	duplicate();
    	reschedule();
    	display_schedule();
}
