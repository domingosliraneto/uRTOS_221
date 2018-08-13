/*---------------------------------------------------o
;M2051 uRTOS V2.20.h (G. Brelaz em 11/11/2016)       o
;sem display de 7 segmentos - S1 em P1.0, S2 em P1.1 o
;+Display LCD com interface de 4 bits - CS em p3.7   o
;+Chamadas as funções de tempo real RTproc_1 e 2     o
;-Removidas funções da Serial (variáveis continuam)  o
;-Removidas funções dos ADC's (variáveis continuam)  o
;+Função delay50 provoca um atrazo de nx50useg       o
;+Funções do LCD: rdLCD4, wrLCD4, num2LCD            o
;+inicialização do LCD em modo interface de 4 bits   o
;---------------------------------------------------*/
#include <8051.h>

__sbit __at (0xB0) RxLed;           //P3.0 RxD LED
__sbit __at (0xB1) TxBip;           //P3.1 TxD buzz
__sfr  __at (0x8F) CLKREG;          //clock register
volatile __sbit __at (0xB2) SwInt;  //P3.2 Int0
volatile __sbit __at (0x90) S1D1;   //P1.0 S1D1
volatile __sbit __at (0x91) S2D2;   //P1.1 S2D2
volatile __sbit __at (0xB7) lcdCS=0;//P3.7 CS   LCD
volatile __sbit __at (0x93) adcCS;  //P1.3 CS   ADC
volatile __sbit __at (0xB4) adcCk;  //P3.4 clk  ADC
volatile __sbit __at (0xB5) adcDt;  //P3.5 dado ADC
volatile __sbit __at (0xB4) DC;     //P3.4 D/C  LCD
volatile __sbit __at (0xB5) RW;     //P3.5 R/W  LCD
volatile __sfr  __at (0x90) LCD;    //porta LCD 8bits
volatile __bit umSeg=0;         //1seg completo
__bit som=0;                    //som ligado
__bit bip=0;                    //bip ligado
__bit tip=0;                    //trava do bip1
__bit RxFlag=0;                 //rxBuff vazio
__bit TxFlag=1;                 //txBuff vazio
__bit Txing=0;                  //enviando do buffer
__bit ad1On=0;                  //ADC1 habilitado
__bit ad2On=0;                  //ADC2 habilitado
__bit Ch01;                     //canal 0/1 ADC
__bit useBuf=0;                 //=1 usa RxB
unsigned char S1=0;             //R1 estado de S1
unsigned char S2=0;             //R2 estado de S2
unsigned char cont=0;           //R3 N x 50µs
unsigned char disp1=0xFF;       //R4 7seg MSD
unsigned char disp2=0xFF;       //R5 7seg LSD
unsigned char cnt100=100;       //R6 100x10ms=1s
unsigned char cnt200=200;       //R7 200 int's
unsigned char x50us;            //var de  delay50
unsigned char freq=8;           //R9 fBip=1/(2*50*freq)
unsigned char RxByte;           //byte recebido
unsigned char TxByte;           //byte transmitir
unsigned char rRxB=0;           //tag leitura RxB
unsigned char wRxB=0;           //tag escrita RxB
unsigned char rTxB=0;           //tag leitura TxB
unsigned char wTxB=0;           //tag escrita TxB
unsigned char RxB[16];          //buffer Rx (recepção)
unsigned char TxB[16];          //buffer Tx (transmissão)
unsigned char algs[5];          //algarismos do byte

//Tabela inicialização do LCD em 4 bits
char __code setLCD4[4]={
    0x28,               //8bits 2lin 5x7
    0x06,               //cursor-> + msg fixa
    0x0E,               //disp +cursor +fixo
    0x01};              //limpa e reseta

#define comand 0
#define letra  1

void RTproc_1();        //a cada 50µs
void RTproc_2();        //a cada 10ms
void delay50();

//----------------------------------------------------
//Vetores de Interrupção são definidos pelo compilador
//----------------------------------------------------
//Interrupção Externa 0: S1D1=P1.0, S2D2=P1.1.

void ext_0 (void) __interrupt(0){
    EX0=0;
    if(!S1D1){
        if (S1<2) S1++;
    }
    else{
        if (S2<2) S2++;
    }
}

//----------------------------------------------------
//Interrupção do Timer 0

void timer_0(void) __interrupt(1) __naked{
__asm
        push   acc                ;salva contexto
        push   PSW                ;
        djnz  _cont,   Cala       ;gerar audio?
        jb    _som,    Bipa       ;tem som?
        jnb   _bip,    Cala       ;tem bip?
Bipa:   mov   _cont,   _freq      ;tempo ½ ciclo
        cpl   _TxBip              ;½ ciclo som
Cala:   djnz  _cnt200, RT1        ;10mS completo?
        mov   _cnt200, #200       ;200*50us
        djnz  _cnt100, Bip1       ;1 seg?
        mov   _cnt100, #100       ;recarga 100
        setb  _umSeg              ;sim, cmpl 1s
Bip1:   jnb   _bip,    Mux        ;se bip ligado
        cpl   _tip                ;|gera de 10ms|
        jb    _tip,    Mux        ;|a 19.95ms de|
        clr   _bip                ;|2*freq*50us.|
Mux:    jb    _S1D1,   D2         ;disp1 ligado?
D1:     mov   a,       #0x02      ;2=filtrada
        cjne  a,       _S1,   D1a ;filtrada?
        jnb   _SwInt,  D1a        ;liberada?
        mov   _S1,     #0x03      ;sim,sobe flag
D1a:    setb  _S1D1               ;S1 deshab
        clr   _S2D2               ;S2 hab
        sjmp  RT2                 ;recarga/sai
D2:     mov   a,       #0x02
        cjne  a,       _S2,   D2a ;filtrada?
        jnb   _SwInt,  D2a        ;liberada?
        mov   _S2,     #0x03      ;sim,sobe flag
D2a:    setb  _S2D2               ;S2 deshab
        clr   _S1D1               ;S1 hab
;
RT2:    setb  _EX0                ;hab. chaves
        lcall _RTproc_2           ;a cada 10ms
        sjmp  SAI
RT1:    lcall _RTproc_1           ;a cada 50µs
SAI:    pop   PSW                 ;recup. contexto
        pop   acc                 ;
        reti
__endasm;
}

//--------------------------------------------------
//Demora tempo múltiplo de 50us
void delay50() __naked{
__asm
        push  acc                 ;salva contexto
        push  PSW                 ;
        mov   a,    _cnt200       ;tempo atual
        clr   c
        subb  a,    _x50us        ;prox tempo
        jnc   Po                  ;negativo?
        add   a,    #200          ;soma 200
Po:     inc   a                   ;nunca zero
Ps:     cjne  a,    _cnt200, Ps   ;espera chegar
        pop   PSW                 ;recup. contexto
        pop   acc                 ;
        ret
__endasm;
}

//--------------------------------------------------
//Grava dado no LCD (8 bits)
void wrLCD8(__bit rs, char val){//comando p/ LCD
    DC=rs; RW=0;                //1dado 0cmd, escrever
    P1_4=(val&0x10); P1_5=(val&0x20);
    P1_6=(val&0x40); P1_7=(val&0x80);
    x50us=2; delay50();         //Tas=100us
    lcdCS=1;                    //para gravar
    x50us=8; delay50();         //PWeh=400us
    lcdCS=0;                    //grava
    x50us=4; delay50();         //Tah=200us
    RW=1; DC=1;                 //linhas ADC em 1
}

//--------------------------------------------------
//Grava dado no LCD (4 bits)
void wrLCD4(__bit rs, char val){//comando p/ LCD
    unsigned char temp;
    temp=(val&0xF0);
    wrLCD8(rs, temp);           //repassa p wrLCD8
    temp=((val<<4)&0xF0);       //pega low nible
    wrLCD8(rs, temp);           //repassa p wrLCD8
}

//--------------------------------------------------
//Escreve o valor numérico de um byte na pos ddRam
//com ponto fixo na posição <dot>.
void char2LCD(char ddRam, unsigned char val, char dot){
    char i=0;
    wrLCD4(comand,ddRam);           //posição número
    algs[0]=0x30+(val/100);
    val=val%100;
    algs[1]=0x30+(val/10);
    algs[2]=0x30+(val%10);
    for(i=0;i<3;i++){
        if(i==dot)wrLCD4(letra, 46);
        wrLCD4(letra, algs[i]);     //coloca número
    }
}

//--------------------------------------------------
//Inicializa Display LCD
void iniLCD4(){                 //inicializa LCD
    unsigned char i;
    unsigned char cmD;
    unsigned char BF=1;
    lcdCS=0;                    //seleciona LCD
    umSeg=0; cnt100=100;        //reinicia umSeg
    while(!umSeg);              //para acomodar LCD
    umSeg=0;
    wrLCD8(comand, 0x30);       //inicia em 8 bits
    while(S1D1); while(S2D2);	//garante 10ms
    wrLCD8(comand, 0x30);       //inicia em 8 bits
    while(S1D1); while(S2D2);	//garante 10ms
    wrLCD8(comand, 0x30);       //inicia em 8 bits
    x50us=4; delay50();         //delay 200us
    wrLCD8(comand, 0x20);       //inicia em 8 bits
    x50us=4; delay50();         //delay 200us
    for(i=0;i<4;i++){           //cmd's 0 a 8'
        cmD=setLCD4[i];         // seleciona comando
        wrLCD4(comand, cmD);
        while(S1D1);
        while(S2D2);            //garante 10ms
    }
}

//--------------------------------------------------
//Inicialização
void inic(void){
    PT0=1;                  //prioridade TC0
    IE=0x93;                //EA+ES+ET0+EX0
    TMOD=0x22;              //TC0=TC1=modo2
    TL0=206;                //conta 50
    TH0=206;                //recarrega
    TL1=243;                //conta 13
    TH1=243;                //recarrega
    PCON=0x80;              //serial x2 (/16)
    SCON=0xC8;              //serial modo 3
    TR0=1;                  //liga tc0
    TR1=1;                  //liga tc1
    iniLCD4();              //inicializa LCD
}

//----------------------------------------------------
//Fim do uRTOS.h