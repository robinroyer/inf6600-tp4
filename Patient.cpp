#include "Patient.hpp"
#include "Helper.hpp"

void *Patient(void *arg){
    
    message msg_i;
    message msg_g;

    while(1){
        usleep(TEMPS_GLYCEMIE);
        mq_receive(msg_Insuline, (char *)&msg_i, sizeof(msg_i), NULL);
        mq_receive(msg_Glucose, (char *)&msg_g, sizeof(msg_g), NULL);
        
        pthread_mutex_lock(&mutGlycemie);
        nivGlycemie = nivGlycemie-(Ki * (double)msg_i.start_stop*INJ - Kg *(double)msg_g.start_stop*INJ);
        std::cout<<nivGlycemie;
        printf("\n");
        pthread_mutex_unlock(&mutGlycemie);
    }
}