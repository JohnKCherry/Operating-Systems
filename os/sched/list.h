#ifndef PROC_LIST_H_
#define PROC_LIST_H_

#define LOW 0
#define HIGH 1

struct process {
	int pNumber;
	int priority;
	pid_t pPid;
	char* execName;
	struct process* next;
};

struct processList{
	int cnt;
	struct process* head;
};

typedef struct process* processT;
typedef struct processList* processListT;


processT initProc(char* execName, int N){
	
	processT proc = malloc(sizeof(struct process));
	if(proc == NULL){
		perror("");
		exit(-1);
	}
	
	proc->pNumber = N;
	proc->priority = LOW;
	proc->pPid = -1;
	proc->execName = malloc((strlen(execName)+1) * sizeof(char));
	strcpy(proc->execName, execName);
	proc->next = NULL;
	
	return proc;
}

processListT initList(){
	
	processListT list = malloc(sizeof(struct processList));
	if(list == NULL){
		perror("");
		exit(-1);
	}
	
	list->cnt = 0;
	list->head = NULL;
	
	return list;
	
}

void addToList(processListT list, processT proc){
	
	list->cnt++;
	if(list->head == NULL) list->head = proc;
	else{ 
		processT tmp = list->head;
		while(tmp->next != NULL) tmp = tmp->next;
		tmp->next = proc;
	}
	
}

void removeFromList(processListT list, processT proc){
	
	processT temp = list->head;
	processT prev = NULL;

	list->cnt--;
	
	if(list->cnt == 0) list->head = NULL;
	else{
		while(temp != proc){
			prev = temp;
			temp = temp->next;
		}
		if (prev){
			prev->next = temp->next;
			temp->pPid = -1;
		}
		else{
			list->head = temp->next;
			temp->pPid = -1;
		}
	}
}

int findNewID(processListT procList){
	
	int id = 1;
	
	while(1){
		processT temp = procList->head;
		int flag=1;
		while(temp != NULL){
			if(temp->pNumber == id){ flag=0; break;}
			temp = temp->next;
		}
		if(flag == 1) return id;
		id++;
	}
}

processT findProcID(processListT list, int id, int * flag){
	
	processT temp = list->head;
	
	while(temp != NULL){
		if(temp->pNumber == id) {*flag = 1; break;}
		temp = temp->next;
	}

	return temp;
	
}

#endif
