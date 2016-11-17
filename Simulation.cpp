#include <cstdlib>
#include <iostream>

#include <stdio.h>
#include <pthread.h>
#include <mqueue.h>
#include <time.h>


#define MAX_NUM_MSG 50

#define HIGH 20
#define NORMAL 10
#define LOW	5

#define GLUCOSE_BAL	"GLUCOSE"
#define INSULINE_BAL "INSULINE"
#define INJECTION_GLUCOSE_BAL "INJECTION_GLUCOSE"
#define INJECTION_INSULINE_BAL "INJECTION_INSULINE"
#define RESET_GLUCOSE_BAL "RESET_GLUCOSE"
#define RESET_INSULINE_BAL "RESET_INSULINE"


#define Ki 1.36;
#define Kg 1.6;

#define MICRO_SECOND 1
#define ONE_SECOND 1000000 * MICRO_SECOND
#define ONE_MINUTE 60 * ONE_SECOND
#define ONE_HOUR 60 * ONE_MINUTE
#define ONE_DAY 24 * ONE_HOUR

#define ANTIBIOTIC_PERIOD 4 * ONE_HOUR
#define RESET_INTERVAL ONE_MINUTE 

typedef struct{
	int start;
	// int stop;
} message;

	pthread_t patientThread, pompeInsulineThread, pompeGlucoseThread, controleGlycemieThread, controleAntibiotiqueThread, controleurSeringueThread;
mqd_t glucoseQueue, insulineQueue, injectionInsulineQueue, injectionGlucoseQueue, resetGlucoseQueue, resetInsulineQueue;

// In order to proctect var
pthread_mutex_t  glycemieLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t  insulineLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t  glucoseLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t  solution1IsRunningLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t  solution2IsRunningLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t  solutionGlucoseIsRunningLock = PTHREAD_MUTEX_INITIALIZER;

// Definition de la memoire partagee
double glycemie = 60;
double txInsuline = 100;
double txGlucose = 100;

bool solution1IsRunning = true;
bool solution2IsRunning = false;
bool solutionGlucoseIsRunning = false;



/*
 *   Handle all the application display
 */
void display(char * msg){
	std::cout << msg << std::endl;	
}


/*
 *   =>
 */
void *patient(void *arg){
	message messageInsuline;
	message messageGlucose;
	double tmp;
	while(1){
		usleep(ONE_SECOND);
		// get insuline and glucose
		mq_receive(insulineQueue, (char *)&messageInsuline, sizeof(messageInsuline), NULL);
		mq_receive(glucoseQueue, (char *)&messageGlucose, sizeof(messageGlucose), NULL);
		
		tmp = (double)messageInsuline.start * Ki - (double)messageGlucose.start * Kg

		pthread_mutex_lock(&glycemieLock);
		glycemie = glycemie - tmp;
		pthread_mutex_unlock(&glycemieLock);
		display("Info : La glycemie vaut : " );
	}
}

/*
 *   =>
 */
void *pompeInsuline(void *arg){
	message messageOut;
	message messageIn;

	while (1){
		usleep(ONE_SECOND);
		mq_receive(injectionInsulineQueue, (char *)&messageIn, sizeof(messageIn), NULL);


		// TODO : protect with mutex
		if (solution1IsRunning || solution2IsRunning){
			pthread_mutex_lock(&insulineLock);
			txInsuline--;
			pthread_mutex_unlock(&insulineLock);

			// testing volume to alert
			if (txInsuline <= 1){				
				messageOut.start = 0;
				mq_send(insulineQueue, (char *)&messageOut, sizeof(messageOut), NORMAL);
				display("ALERT : arret injection d'insuline => 1%");
				//  TODO handle the 2 seringue

			}else if(txInsuline <= 5){
				display("ALERT : niveau d'insuline bas => 5%");				
			} else{
				// => working fine
			}

			// asking to stop
			if (!messageIn.start){
				messageOut.start = 0;
				mq_send(insulineQueue, (char *)&messageOut, sizeof(messageOut), NORMAL);
				display("INFO : stop injection insuline");
				
			}
		} else {
			if (messageIn.start){
				messageOut.start = 1;
				mq_send(insulineQueue, (char *)&messageOut, sizeof(messageOut), NORMAL);
				display("INFO : begin injection insuline");
			}

		}
	}
}

/*
 *   =>
 */
void *pompeGlucose(void *arg){
	message messageOut;
	message messageIN;

	while (1){
		usleep(ONE_SECOND);
		mq_receive(injectionGlucoseQueue, (char *)&messageIN, sizeof(messageIN), NULL);

		if (txGlucose > 1){
			if (messageIN.start){
				messageOut.start = 1;
				pthread_mutex_lock(&glucoseLock);
				mq_send(glucoseQueue, (char *)&messageOut, sizeof(messageOut), NORMAL);
				txGlucose = txGlucose - 1;
				pthread_mutex_unlock(&glucoseLock);
				display("Injection de glucose : start");
			}
			else{
				messageOut.start = 0;
				pthread_mutex_lock(&glucoseLock);
				mq_send(glucoseQueue, (char *)&messageOut, sizeof(messageOut), NORMAL);
				txGlucose = txGlucose;
				pthread_mutex_unlock(&glucoseLock);				
				display("Injection de glucose : stop");
			}
			if(txGlucose < 6){
				display("ALERT : niveau d'insuline bas => 5%");
			}
		}
		else{
			messageOut.start = 0;
			pthread_mutex_lock(&glucoseLock);
			mq_send(glucoseQueue, (char *)&messageOut, sizeof(messageOut), NORMAL);
			txGlucose = txGlucose;
			pthread_mutex_unlock(&glucoseLock);

			display("ALERT : arret injection de glucose => 1%");
		}
	}
}

/**
 * Le controlleur envoit a inteval regulier des messages dans les 2 BALs de gestion insuline et glucose
 */
void *controleur(void *arg){
	double niveauGlycemieLocal;

	message controle_Insuline;
	message controle_Glucose;

	while (1){
		usleep(ONE_SECOND);
		pthread_mutex_lock(&glycemieLock);
		niveauGlycemieLocal = glycemie;
		pthread_mutex_unlock(&glycemieLock);

		if (niveauGlycemieLocal < 60){
			controle_Insuline.start = 0;
			controle_Glucose.start = 1;
		}
		else{
			if (niveauGlycemieLocal > 120){
				controle_Insuline.start = 1;
				controle_Glucose.start = 0;
			}
			else{
				controle_Insuline.start = 0;
				controle_Glucose.start = 0;
			}
		}
		mq_send(injectionGlucoseQueue, (char *)&controle_Glucose, sizeof(controle_Glucose), NORMAL);
		mq_send(injectionInsulineQueue, (char *)&controle_Insuline, sizeof(controle_Insuline), NORMAL);

	}
}


/*
 * Injection handler
 */
void *controleAntibiotique(void *arg){
	int i=0;
	while (1){
		if(i%6==0){
			display("Info : injection anticoagulant");
			usleep(3 * ONE_MINUTE);
			display("Info : arret injection anticoagulant apres 3 minutes" );
		}
			display("Info : injection antibiotique");
			usleep(ONE_MINUTE);
			display("Info : arret injection antibiotique");
		i++;
		usleep(ANTIBIOTIC_PERIOD);
	}
}


/*
 * TODO 
 */
void *controleurSeringue(void *arg){
	
	while(1){
		usleep(RESET_INTERVAL);
		pthread_mutex_lock(&glucoseLock);
		if(txGlucose<1)
			txGlucose=100;
		pthread_mutex_unlock(&glucoseLock);
		
		pthread_mutex_lock(&insulineLock);
		if(txInsuline<1)
			txInsuline = 100;
		pthread_mutex_unlock(&insulineLock);
	}	
}


/*
 *   =>
 */
void cleanUp(void){

	// CLEAN UP
	pthread_join(pompeInsulineThread, NULL);
	pthread_join(patientThread, NULL);
	pthread_join(pompeGlucoseThread, NULL);
	pthread_join(controleAntibiotiqueThread, NULL);
	pthread_join(controleGlycemieThread, NULL);
	pthread_join(patientThread, NULL);
	pthread_join(controleurSeringueThread, NULL);
	
	mq_close(glucoseQueue);
	mq_close(injectionInsulineQueue);
	mq_close(resetGlucoseQueue);
	mq_close(insulineQueue);
	mq_close(injectionGlucoseQueue);
	mq_close(resetInsulineQueue);
	
	mq_unlink(GLUCOSE_BAL);
	mq_unlink(INJECTION_GLUCOSE_BAL);
	mq_unlink(RESET_GLUCOSE_BAL);
	mq_unlink(INJECTION_INSULINE_BAL);
	mq_unlink(RESET_INSULINE_BAL);
	mq_unlink(INSULINE_BAL);
}


/*
 *   =>
 */
void run(void){
	pthread_attr_t attrib;
	setprio(0,20);
	struct sched_param mySchedParam;
	struct mq_attr mqattr;
	
	// BOITE AUX LETTRES
	mqattr.mq_maxmsg = MAX_NUM_MSG;
	mqattr.mq_msgsize = sizeof (message);
	
	glucoseQueue = mq_open(GLUCOSE_BAL, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR, &mqattr);
	insulineQueue = mq_open(INSULINE_BAL, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR, &mqattr);
	injectionInsulineQueue = mq_open(INJECTION_INSULINE_BAL, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR, &mqattr);
	injectionGlucoseQueue = mq_open(INJECTION_GLUCOSE_BAL, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR, &mqattr);
	resetInsulineQueue = mq_open(RESET_INSULINE_BAL, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR, &mqattr);
	resetGlucoseQueue = mq_open(RESET_GLUCOSE_BAL, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR, &mqattr);
	
	// set up
	setprio(0, 20);
	pthread_attr_init (&attrib);
	pthread_attr_setinheritsched(&attrib, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy (&attrib, SCHED_FIFO);
	
	//  nos differetns threads
	
	mySchedParam.sched_priority = NORMAL;
	pthread_attr_setschedparam(&attrib, &mySchedParam);
	pthread_create(&patientThread,&attrib, patient,NULL);
	mySchedParam.sched_priority = NORMAL;
	pthread_attr_setschedparam(&attrib, &mySchedParam);
	pthread_create(&pompeInsulineThread,&attrib, pompeInsuline,NULL);
	mySchedParam.sched_priority = HIGH;
	pthread_attr_setschedparam(&attrib, &mySchedParam);
	pthread_create(&controleGlycemieThread,& attrib, controleur,NULL);
	mySchedParam.sched_priority = HIGH;
	pthread_attr_setschedparam(&attrib, &mySchedParam);
	pthread_create(&pompeGlucoseThread,&attrib, pompeGlucose,NULL);
	mySchedParam.sched_priority = HIGH;
	pthread_attr_setschedparam(&attrib, &mySchedParam);
	pthread_create(&controleAntibiotiqueThread,&attrib, controleAntibiotique,NULL);
	mySchedParam.sched_priority = LOW;
	pthread_attr_setschedparam(&attrib, &mySchedParam);
	pthread_create(&controleurSeringueThread,&attrib, controleurSeringue,NULL);
}


/*
 *   =>
 */
int main(int argc, char *argv[]) {
	run();
	cleanUp();
	return 1;
}
