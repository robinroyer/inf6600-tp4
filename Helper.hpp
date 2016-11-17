#ifndef _HELPER_H_
#define _HELPER_H_

/**
 *	This file hold all our application macros to avoid circular inclusion and heavy coupling
 */


#define URGENT      50
#define NORMAL      25
#define BAS         10

#define HYPERGLYCEMIE                   160
#define HYPOGLYCEMIE                    50

#define START                           1
#define STOP                            0
#define INJ                             2
#define TEMPS_GLYCEMIE                  10000
#define TEMPS_RESET                     10000
#define TEMPS_ANTIBIO                   100000
#define TEMPS_INJ_ANTICOA               1200
#define TEMPS_INJ_ANTIBIO               1000

#define MAX_NUM_MSG 50

#define MQ_GLUCOSE "MMQ_GLUCOSE"
#define MQ_INSULINE "MQ_INSULINE"
#define MQ_INJECTION_GLUCOSE "MQ_INJECTION_GLUCOSE"
#define MQ_INJECTION_INSULINE "MQ_INJECTION_INSULINE"
#define MQ_RESET_GLUCOSE "MQ_RESET_GLUCOSE"
#define MQ_RESET_INSULINE "MQ_RESET_INSULINE"

pthread_mutex_t  mutGlycemie = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t  mutInsuline = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t  mutGlucose = PTHREAD_MUTEX_INITIALIZER;

double nivGlycemie = 40;
double nivInsuline = 100;
double nivGlucose = 100;

typedef struct{
	int start_stop;
} message;



#endif
