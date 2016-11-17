#include "PompeGlucose.hpp"
#include "Helper.hpp"



void *pompeGlucose(void *arg){
	message msg_g;
	message msg_glu;

	while (1){
		usleep(TEMPS_GLYCEMIE);
		mq_receive(msg_injection_Glucose, (char *)&msg_glu, sizeof(msg_glu), NULL);

		pthread_mutex_lock(&mutGlucose);
		if (nivGlucose > 0){
			if (msg_glu.start_stop){
				msg_g.start_stop = START;
				mq_send(msg_Glucose, (char *)&msg_g, sizeof(msg_g), NORMAL);
				nivGlucose = nivGlucose - INJ;
				printf("Injection de glucose : start\n");
			}
			else{
				msg_g.start_stop = STOP;
				mq_send(msg_Glucose, (char *)&msg_g, sizeof(msg_g), NORMAL);
				nivGlucose = nivGlucose;
				printf("Injection de glucose : stop\n");
			}
		}
		else{
			msg_g.start_stop = STOP;
			mq_send(msg_Glucose, (char *)&msg_g, sizeof(msg_g), NORMAL);
			nivGlucose = nivGlucose;
			printf("Injection de glucose : NON niveau bas\n");
		}
		printf("niv glucose : ");
		std::cout<<nivGlucose;
		printf("\n");
		pthread_mutex_unlock(&mutGlucose);
	}
}