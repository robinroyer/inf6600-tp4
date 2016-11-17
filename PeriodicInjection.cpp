#include "signal.h"
#include "PeriodicInjection.hpp"
#include "Helper.hpp"


void *antibioticInjection(void *arg){
	int i=0;
	while (1){
		usleep(TEMPS_ANTIBIO);
		if(i==24){
			i=0;
		}
		if(i==0){
			printf("Injection Anticoagulant\n");
			usleep(TEMPS_INJ_ANTICOA);
			printf("Fin injection Anticoagulant\n");
			printf("Injection Antibiotique\n");
			usleep(TEMPS_INJ_ANTIBIO);
			printf("Fin injection Antibiotique\n");
		}
		if(i==6||i==12||i==18){
			printf("Injection Antibiotique\n");
			usleep(TEMPS_INJ_ANTIBIO);
			printf("Fin injection Antibiotique\n");
		}
		i++;
		printf("Il est ");
		std:: cout<<i;
		printf(" h\n");
	}
}