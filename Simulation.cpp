// 
//      INF6600  -  Polytechnique
// 
// 		Author: Robin Royer
// 		Date: 20/11/16

#include <cstdlib>
#include <iostream>

#include <stdio.h>
#include <pthread.h>
#include <mqueue.h>
#include <time.h>

#define MAX_NUM_MSG 50

#define HIGH 5
#define NORMAL 10
#define LOW	20

#define GLUCOSE_BAL	"GLUCOSE"
#define INSULINE_BAL "INSULINE"
#define INJECTION_GLUCOSE_BAL "INJECTION_GLUCOSE"
#define INJECTION_INSULINE_BAL "INJECTION_INSULINE"
#define RESET_GLUCOSE_BAL "RESET_GLUCOSE"
#define RESET_INSULINE_BAL "RESET_INSULINE"

#define MICRO_SECOND 1
#define ONE_SECOND 1000000 * MICRO_SECOND
#define ONE_MINUTE 60 * ONE_SECOND
#define ONE_HOUR 60 * ONE_MINUTE
#define ONE_DAY 24 * ONE_HOUR

#define ANTIBIOTIC_PERIOD 4 * ONE_HOUR
#define RESET_INTERVAL ONE_MINUTE

typedef struct{
	int start;
} message;

	pthread_t patientThread, pompeInsulineThread, pompeGlucoseThread, controleGlycemieThread, controleAntibiotiqueThread, resetThread;
mqd_t glucoseInjectionQueue, insulineInjectionQueue, insulineSeringueQueue, glucoseSeringueQueue;

// Mutex in order to proctect var
pthread_mutex_t  glycemieLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t  insulineLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t  glucoseLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t  glucoseIsRunningLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t  insulineIsRunningLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t  isResetLock = PTHREAD_MUTEX_INITIALIZER;

// Definition des variables partagees
double glycemie = 300;

double txInsuline = 100;
double txGlucose = 100;

bool glucoseIsRunning = false;
bool insulineIsRunning  = false;
bool isReset = true;



/*
 *   Handle all the application display
 */
void display(char * msg){
	std::cout << msg << std::endl;	
}

/*
 *   Handle all the application display
 */
void printValue(char * msg, double value){
	std::cout << msg <<  value << std::endl;	
}

/**
 * Safe read operation on shared variable
 */
bool safeBoolRead(bool* myBool, pthread_mutex_t *lock){
	bool tmp;
	pthread_mutex_lock(lock);
	tmp = *myBool;
	pthread_mutex_unlock(lock);
	return tmp;
}

/**
 * Safe read operation on shared variable
 */
double safeDoubleRead(double* myDouble, pthread_mutex_t *lock){
	double tmp;
	pthread_mutex_lock(lock);
	tmp = *myDouble;
	pthread_mutex_unlock(lock);
	return tmp;
}

/**
 * Safe write operation on shared variable
 */
void safeBoolWrite(bool* myBool, bool value, pthread_mutex_t *lock){
	pthread_mutex_lock(lock);
	*myBool = value;
	pthread_mutex_unlock(lock);
}

/**
 * Safe write operation on shared variable
 */
void safeDoubleWrite(double* myDouble, double value, pthread_mutex_t *lock){
	pthread_mutex_lock(lock);
	*myDouble = value;
	pthread_mutex_unlock(lock);
}


/*
 *  Patient evolution of glycemia
 */
void *patient(void *arg){
	message messageInsuline;
	message messageGlucose;

	double tmp;
	while(1){
		usleep(ONE_SECOND);
		// get insuline and glucose
		mq_receive(insulineInjectionQueue, (char *)&messageInsuline, sizeof(messageInsuline), NULL);
		mq_receive(glucoseInjectionQueue, (char *)&messageGlucose, sizeof(messageGlucose), NULL);
		
		tmp = (double)messageInsuline.start * 1.36 - (double)messageGlucose.start * 1,6;

		pthread_mutex_lock(&glycemieLock);
		glycemie = glycemie - tmp;
		pthread_mutex_unlock(&glycemieLock);

		printValue("Info : La glycemie vaut : ", glycemie);
	}
}

/*
 *   Insuline pump handler
 */
void *pompeInsuline(void *arg){
	message messageOut;
	message messageIn;

	while (1){
		usleep(ONE_SECOND);
		mq_receive(insulineSeringueQueue, (char *)&messageIn, sizeof(messageIn), NULL);

		printValue("insuline : ", txInsuline);


		if (safeBoolRead(&insulineIsRunning, &insulineIsRunningLock) && safeDoubleRead(&txInsuline, &insulineLock) >= 0){
			messageOut.start = 1;
			safeDoubleWrite(&txInsuline, txInsuline -1.0, &insulineLock);

			// testing volume to alert
			if (safeDoubleRead(&txInsuline, &insulineLock) <= 1){			

				if(safeBoolRead(&isReset, &isResetLock)){
					display("INFO : Changement de seringue d'insuline => 1%");
					safeDoubleWrite(&txInsuline, 100.0, &insulineLock);
					safeBoolWrite(&isReset, false, &isResetLock);
				} else{
					display("ALERT : arret injection d'insuline => 1%");
					messageOut.start = 0;
					safeBoolWrite(&insulineIsRunning, false, &insulineIsRunningLock);
				}

			}else if(safeDoubleRead(&txInsuline, &insulineLock) <= 5){
				display("ALERT : niveau d'insuline bas => 5%");				
			} else{
				// => working fine
			}

			// asking to stop
			if (!messageIn.start){
				messageOut.start = 0;				
				safeBoolWrite(&insulineIsRunning, false, &insulineIsRunningLock);
				display("INFO : stop injection insuline");				
			}
		} else {

			if (messageIn.start){
				// asking to start
				messageOut.start = 1;
				safeBoolWrite(&insulineIsRunning, true, &insulineIsRunningLock);
				display("INFO : begin injection insuline");
			}else{
				messageOut.start = 0;
				safeBoolWrite(&insulineIsRunning, false, &insulineIsRunningLock);
			}
		}
		mq_send(insulineInjectionQueue, (char *)&messageOut, sizeof(messageOut), NORMAL);
	}
}

/*
 *   Glucose pump handler
 */
void *pompeGlucose(void *arg){
	message messageOut;
	message messageIn;

	while (1){
		usleep(ONE_SECOND);
		mq_receive(glucoseSeringueQueue, (char *)&messageIn, sizeof(messageIn), NULL);
		printValue("glucose : ", safeDoubleRead(&txGlucose, &glucoseLock));
		messageOut.start = 0;

		if (safeBoolRead(&glucoseIsRunning, &glucoseIsRunningLock)  && safeDoubleRead(&txGlucose, &glucoseLock) > 0){
			messageOut.start = 1;
			safeDoubleWrite(&txGlucose, txGlucose - 1.0, &glucoseLock);
			// testing volume to alert
			if (safeDoubleRead(&txGlucose, &glucoseLock) <= 1){				
				display("ALERT : arret injection de glucose => 1%");
				messageOut.start = 0;	
				safeBoolWrite(&glucoseIsRunning, false, &glucoseIsRunningLock);

			} else{
				// => working fine
			}

			// asking to stop
			if (!messageIn.start){
				messageOut.start = 0;
				safeBoolWrite(&glucoseIsRunning, false, &glucoseIsRunningLock);
				display("INFO : stop injection insuline");
			}
		} else if (safeDoubleRead(&txGlucose, &glucoseLock) > 0){
			if (messageIn.start){
				// asking to start
				safeBoolWrite(&glucoseIsRunning, true, &glucoseIsRunningLock);
				messageOut.start = 1;
				display("INFO : debut injection insuline");
			}else{
				messageOut.start = 0;
			}
		}
		mq_send(glucoseInjectionQueue, (char *)&messageOut, sizeof(messageOut), NORMAL);
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
		niveauGlycemieLocal = safeDoubleRead(&glycemie, &glycemieLock);

		if (niveauGlycemieLocal < 60){
			display("ALERT : Le patient est en hypoglycemie!");
			controle_Insuline.start = 0;
			controle_Glucose.start = 1;
		}
		else if (niveauGlycemieLocal > 120){
			display("ALERT : Le patient est a trop de sucre dans le sang!");
			controle_Insuline.start = 1;
			controle_Glucose.start = 0;
		}
		else {
			display("Info : Le patient est en etat normal!");
			controle_Insuline.start = 0;
			controle_Glucose.start = 0;
		}
		mq_send(glucoseSeringueQueue, (char *)&controle_Glucose, sizeof(controle_Glucose), NORMAL);
		mq_send(insulineSeringueQueue, (char *)&controle_Insuline, sizeof(controle_Insuline), NORMAL);
	}
}


/*
 * Injection handler
 */
void *controleAntibiotique(void *arg){
	int i=0;
	while (1){
		if(i%6 == 0){
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
 * Filling ampoule
 */
void *controleurReset(void *arg){	
	while(1){
		usleep(RESET_INTERVAL);

		safeDoubleWrite(&txGlucose, 100, &glucoseLock);
		safeBoolWrite(&isReset, true, &isResetLock);
	}	
}


/*
 *  Avoid memory leak
 */
void cleanUp(void){

	// CLEAN UP
	pthread_join(pompeInsulineThread, NULL);
	pthread_join(patientThread, NULL);
	pthread_join(controleAntibiotiqueThread, NULL);
	pthread_join(pompeGlucoseThread, NULL);
	pthread_join(patientThread, NULL);
	pthread_join(controleGlycemieThread, NULL);
	pthread_join(resetThread, NULL);
	
	mq_close(glucoseInjectionQueue);
	mq_close(insulineSeringueQueue);
	mq_close(insulineInjectionQueue);
	mq_close(glucoseSeringueQueue);
	
	mq_unlink(GLUCOSE_BAL);
	mq_unlink(INJECTION_GLUCOSE_BAL);
	mq_unlink(INJECTION_INSULINE_BAL);
	mq_unlink(INSULINE_BAL);
}


/*
 *  Init struct memories and threads
 */
void run(void){

	//TODO handle ctrl+C signal and think about repetitive thread

	pthread_attr_t attrib;
	setprio(0,20);
	struct sched_param mySchedParam;
	struct mq_attr queueAttribut;
	
	// BOITE AUX LETTRES
	queueAttribut.mq_maxmsg = MAX_NUM_MSG;
	queueAttribut.mq_msgsize = sizeof(message);
	
	glucoseInjectionQueue = mq_open(GLUCOSE_BAL, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR, &queueAttribut);
	insulineInjectionQueue = mq_open(INSULINE_BAL, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR, &queueAttribut);
	insulineSeringueQueue = mq_open(INJECTION_INSULINE_BAL, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR, &queueAttribut);
	glucoseSeringueQueue = mq_open(INJECTION_GLUCOSE_BAL, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR, &queueAttribut);
	
	// set up
	setprio(0, 20);
	pthread_attr_init (&attrib);
	pthread_attr_setinheritsched(&attrib, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy (&attrib, SCHED_FIFO);
	
	//  Nos differentes threads
	mySchedParam.sched_priority = HIGH;
	pthread_attr_setschedparam(&attrib, &mySchedParam);
	pthread_create(&patientThread,&attrib, patient,NULL);
	mySchedParam.sched_priority = HIGH;
	pthread_attr_setschedparam(&attrib, &mySchedParam);
	pthread_create(&pompeInsulineThread,&attrib, pompeInsuline,NULL);
	mySchedParam.sched_priority = HIGH;
	pthread_attr_setschedparam(&attrib, &mySchedParam);
	pthread_create(&controleGlycemieThread,& attrib, controleur,NULL);
	mySchedParam.sched_priority = HIGH;
	pthread_attr_setschedparam(&attrib, &mySchedParam);
	pthread_create(&pompeGlucoseThread,&attrib, pompeGlucose,NULL);
	mySchedParam.sched_priority = NORMAL;
	pthread_attr_setschedparam(&attrib, &mySchedParam);
	pthread_create(&controleAntibiotiqueThread,&attrib, controleAntibiotique,NULL);
	mySchedParam.sched_priority = LOW;
	pthread_attr_setschedparam(&attrib, &mySchedParam);
	pthread_create(&resetThread,&attrib, controleurReset,NULL);
}


/*
 *   main
 */
int main(int argc, char *argv[]) {
	run();
	cleanUp();
	return 1;
}
