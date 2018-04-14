#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>



#define atomic_xchg(A,B)  __asm__ __volatile__("xchg %1,%0" :"=r" (A) :"m" (B), "r" (A) :"memory");	//Macro de atomicidad
#define CICLOS 10		//Numero de veces que entra cada país												
#define MAXPROCESS 3	//Numero de procesos máximos en la cola



typedef struct _QUEUE {			//Utilizamos la estructura de cola de la práctica 4
	int elements[MAXPROCESS];
	int head;
	int tail;
} QUEUE;

typedef struct SEMAPHORE {		//Definimos la estructura de nuestro semaforo
    int contador;				//Contador para saber si la zonar crítica esta llena
    QUEUE blockedQUEUE;			//Cola de bloqueados a donde mandaremos el PID del proceso
    int g;						//Variable global para atomicidad
} sem;

//Declaracion de funciones de colas de practica 4
void _initqueue(QUEUE *q);
void _enqueue(QUEUE *q,int val);
int _dequeue(QUEUE *q);
int _emptyq(QUEUE *q);

//Declaracion de funciones de semaforo
void _initsem(sem *s, int val);
void _waitsem(sem *s);
void _signalsem(sem *s);



char *pais[3]={"Peru","Bolvia","Colombia"};	
sem *s;


void proceso(int i)
	{
	int k;
	for(k=0;k<CICLOS;k++)
		{
		_waitsem(&(*s));				//Enviamos la direccion de nuestro semáforo global (memoria compartida) al wait
		printf("Entra %s",pais[i]);
		fflush(stdout);
		sleep(rand()%3);
		printf("- %s Sale\n",pais[i]);
		_signalsem(&(*s));				//Enviamos la direccion de nuestro semáforo global (memoria compartida) al signal

		// Espera aleatoria fuera de la sección crítica
		sleep(rand()%3);
		}
	exit(0);
	// Termina el proceso
	}
int main()
	{
	int pid;
	int status;
	int shmid;
	int args[3];
	int i;
	void *thread_result;


	// Solicitar memoria compartida
	shmid=shmget(0x1234,sizeof(s),0666|IPC_CREAT);
	if(shmid==-1)
		{
		perror("Error en la memoria compartida\n");
		exit(1);
		}
	// Conectar la variable a la memoria compartida
	s=shmat(shmid,NULL,0);
	if(s==NULL)
		{
		perror("Error en el shmat\n");
		exit(2);
		}

	_initsem(&(*s),1); //Iniciamos nuestro semáforo global (memoria compartida)

	srand(getpid());

	for(i=0;i<3;i++)
		{
		// Crea un nuevo proceso hijo que ejecuta la función proceso()
		pid=fork();
		if(pid==0)
			proceso(i);
		else{

		}
		}
	for(i=0;i<3;i++)
		pid = wait(&status);
	// Eliminar la memoria compartida
	shmdt(s);
	}




	void _initsem(sem *s, int val){ //Recibe nuestro semaforo global y el un val que indica cuantos procesos pueden entrar a la zona crítica
		s->contador=val;			//Asignamos al contador el valor de val
		s->g=0;						//Asignamos a nuestra variable global de atomicidad 0
		_initqueue(&s->blockedQUEUE);//Iniciamos la cola de bloqueados
	}

	void _waitsem(sem *s){ 			//Recibe nuestro semaforo global

		int *la;							//Apuntador local 
		la = (int *) malloc (sizeof(int));	//Reservamos espacio de memoria
		*la=1;								//Le asignamos el valor de 1

		do { atomic_xchg(*la,s->g); } while(*la!=0);	//Mientras *la sea diferente a cero nos ciclamos
		s->contador--;									//Decrementamos

		if(	s->contador<0){								//Si el contador es menor a cero significa que la zona critica está llena
			_enqueue(&s->blockedQUEUE, getpid());		//Enviamos el PID del proceso a la cola de bloqueados
			*la=1;										//Asignamos cero a la variable de atomicidad liberar alguna wait o signal
			s->g=0;
			kill(getpid(),SIGSTOP);						//Bloqueamos el proceso actual
		}
		else{
			*la=1;										//Asignamos cero a la variable de atomicidad liberar alguna wait o signal
			s->g=0;
		}
		
	}

	void _signalsem(sem *s){					//Recibe nuestro semaforo global
		
		int *la;								//Apuntador local 
		la = (int *) malloc (sizeof(int));		//Reservamos espacio de memoria
		int pidProccess;						//Variable donde recibiremos el PID del proceso

		*la=1;									//Le asignamos el valor de 1
			
		do { atomic_xchg(*la,s->g); } while(*la!=0);	//Mientras *la sea diferente a cero nos ciclamos

		s->contador++;									//Incrementamos
		if(	s->contador<=0){							//Si el contador es menor o igual a cero desbloqueamos un proceso de la cola de bloqueados
			pidProccess=_dequeue(&s->blockedQUEUE);
			kill(pidProccess,SIGCONT);
		}
		*la=1;
		s->g=0;											//Asignamos cero a la variable de atomicidad liberar alguna wait o signal

	}


	//Funciones de colas de la práctica 4

		void _initqueue(QUEUE *q)
	{
		q->head=0;
		q->tail=0;
	}
	

	void _enqueue(QUEUE *q,int val)
	{
		q->elements[q->head]=val;
		// Incrementa al apuntador
		q->head++;
		q->head=q->head%MAXPROCESS;
	}


	int _dequeue(QUEUE *q)
	{
		int valret;
		valret=q->elements[q->tail];
		// Incrementa al apuntador
		q->tail++;
		q->tail=q->tail%MAXPROCESS;
		return(valret);
	}

	int _emptyq(QUEUE *q)
	{
		return(q->head==q->tail);
	}

	