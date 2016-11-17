#include "Capteur.hpp"
#include "Helper.hpp"

void *capteur(void *arg){
	double niveauGlycemieLocal;

	message controle_Insuline;
	message controle_Glucose;

	while (1){
		usleep(10*TEMPS_GLYCEMIE);//periode execution
		pthread_mutex_lock(&mutGlycemie);
		niveauGlycemieLocal = nivGlycemie;
		pthread_mutex_unlock(&mutGlycemie);

		if (niveauGlycemieLocal < HYPOGLYCEMIE){
			controle_Insuline.start_stop = STOP;
			controle_Glucose.start_stop = START;
		}
		else{
			if (niveauGlycemieLocal > HYPERGLYCEMIE){
				controle_Insuline.start_stop = START;
				controle_Glucose.start_stop = STOP;
			}
			//else{
			//	controle_Insuline.start_stop = STOP;
			//	controle_Glucose.start_stop = STOP;
			//}
		}
		mq_send(msg_injection_Glucose, (char *)&controle_Glucose, sizeof(controle_Glucose), NORMAL);
		mq_send(msg_injection_Insuline, (char *)&controle_Insuline, sizeof(controle_Insuline), NORMAL);

	}
}