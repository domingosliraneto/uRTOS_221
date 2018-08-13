/*---------------------------------------------------o
;M2051 Lab020 Prog06 posLCD.c (01/09/2016)           o
;Posiciona títulos e valores numéricos no LCD        o
;Executa contagem ao pressionar S1 e para com S2     o
;Usando µRTOS v2.21                                  o
;---------------------------------------------------*/
#include <uRTOS_2.21.h>

//Titulos do uRTOS
char __code Tit1[10]="Kit M2051";   //título 1ª linha
char __code Tit2[12]="uRTOS_V2.21"; //título 2ª linha
void RTproc_1(){}                   //a cada 50uS
void RTproc_2(){}                   //a cada 10ms

void main (void){
    signed char i=0;                //variável de loop
    unsigned char Num=0;            //contador
    __bit conta=0;                  //semáforo contagem
    inic();                         //inicializa uRTOS
    for(i=0;i<9;i++){
        wrLCD4(letra, Tit1[i]);     //primeira linha
    }
    wrLCD4(comand,0xC0);            //posiciona linha 2
    for(i=0;i<11;i++){
        wrLCD4(letra, Tit2[i]);     //segunda linha
    }
    while(1){
        if(S1==3){                  //chave 1
            S1=0; conta=1; RxLed=0; //liga flag, RxLed
        }
        if(S2==3){                  //chave 2
            S2=0; conta=0; RxLed=1; //apaga flag, RxLed
        }
        if(conta) Num++;            //conta se flag ligada
        while(!umSeg); umSeg=0;     //a cada um segundo
        char2LCD(0xCC, Num, 1);     //envia Num p/ LCD
    }
}