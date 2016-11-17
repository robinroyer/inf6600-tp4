#include "PompeInsuline.hpp"
#include "Helper.hpp"


void *pompeInsuline(void *arg){
	message msg_i;
	message msg_inj;

	while (1){
		usleep(TEMPS_GLYCEMIE);
		mq_receive(msg_injection_Insuline, (char *)&msg_inj, sizeof(msg_inj), NULL);

		pthread_mutex_lock(&mutInsuline);
		if (nivInsuline > 0){
			if (msg_inj.start_stop){
				msg_i.start_stop = START;
				mq_send(msg_Insuline, (char *)&msg_i, sizeof(msg_i), NORMAL);
				nivInsuline = nivInsuline - INJ;
				printf("Injection d'insuline : start\n");
			}
			else{
				msg_i.start_stop = STOP;
				mq_send(msg_Insuline, (char *)&msg_i, sizeof(msg_i), NORMAL);
				nivInsuline = nivInsuline;
				printf("Injection d'insuline : stop\n");
			}
		}
		else{
			msg_i.start_stop = STOP;
			mq_send(msg_Insuline, (char *)&msg_i, sizeof(msg_i), NORMAL);
			nivInsuline = nivInsuline;
			printf("Injection d'insuline : NON niveau bas\n");
		}
		printf("niv insuline : ");
		std::cout<<nivInsuline;
		printf("\n");
		pthread_mutex_unlock(&mutInsuline);
	}
}