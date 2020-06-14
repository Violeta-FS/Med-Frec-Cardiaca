//MEDIDOR DE FRECUENCIA CARDIACA
#include <LiquidCrystal_I2C.h>                    //Agregar libreria de LCD con I2C
LiquidCrystal_I2C lcd(0x27,16,2);                //Inicia la LCD 16x2
int pulsoPin = 0;                                //Sensor de pulso conectado al puerto C0
                                 //*Las variables volatile son usadas cuando estas no son fijas, es decir pueden cambiar de cualquier momento para las interrupciones  
volatile int BPM;                                //Se declara la variable de pulsaciones por minuto

volatile int Signal;                             //Se declara la variable entrada de datos del sensor
volatile int IBI = 600;                          //Se declara la variable de tiempo entre pulsaciones
volatile boolean pulso = false;                  //Será verdadero cuando la onda de pulsos es alta, falso cuando es baja
volatile boolean QS = false;                     //Será verdadero cuando el arduino busca un pulso del corazón
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
                                              //+++++++VARIABLES PARA INTERRUPPCIONES+++++++
                                              
volatile int rate[10];                      // array to hold last ten IBI values
volatile unsigned long sampleCounter = 0;          // Determina tiempo de pulso
volatile unsigned long lastBeatTime = 0;           // used to find IBI
volatile int P =512;                      // Pico alto del pulso
volatile int T = 512;                     // Pico bajo de la onda
volatile int thresh = 512;                // Momentos instantaneos de latidos cardiacos
volatile int amp = 100;                   // Mantiene la amplitud de la forma de onda del pulso
volatile boolean firstBeat = true;        // used to seed rate array so we startup with reasonable BPM
volatile boolean secondBeat = false;      // used to seed rate array so we startup with reasonable BPM

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
                                               //+++++++++++FUNCION DE INTERRUPCIONES+++++++++++
void interruptSetup(){
 // Se inicia timer2 para tener interrupción cada 2ms
  TCCR2A = 0x02;     // DESACTIVE PWM EN LOS PINES DIGITALES 3 Y 11, Y VAYA AL MODO CTC
  TCCR2B = 0x06;     // DON'T FORCE COMPARE, 256 PRESCALER 
  OCR2A = 0X7C;      // SET THE TOP OF THE COUNT TO 124 FOR 500Hz SAMPLE RATE
  TIMSK2 = 0x02;     // ENABLE INTERRUPT ON MATCH BETWEEN TIMER2 AND OCR2A
  sei();       
}


// THIS IS THE TIMER 2 INTERRUPT SERVICE ROUTINE. 
// Timer 2 makes sure that we take a reading every 2 miliseconds
ISR(TIMER2_COMPA_vect){                         // triggered when Timer2 counts to 124
  cli();                                      // disable interrupts while we do this
  Signal = analogRead(pulsoPin);              // read the Pulse Sensor 
  sampleCounter += 2;                         // keep track of the time in mS with this variable
  int N = sampleCounter - lastBeatTime;       // monitor the time since the last beat to avoid noise

    //  find the peak and trough of the pulse wave
  if(Signal < thresh && N > (IBI/5)*3){       // avoid dichrotic noise by waiting 3/5 of last IBI
    if (Signal < T){                        // T is the trough
      T = Signal;                         // keep track of lowest point in pulse wave 
      
    }
  }

  if(Signal > thresh && Signal > P){          // thresh condition helps avoid noise
    P = Signal;                             // P is the peak
  }                                        // keep track of highest point in pulse wave

  //  NOW IT'S TIME TO LOOK FOR THE HEART BEAT
  // signal surges up in value every time there is a pulse
  if (N > 250){                                   // avoid high frequency noise
    if ( (Signal > thresh) && (pulso == false) && (N > (IBI/5)*3) ){        
      pulso = true;                               // set the Pulse flag when we think there is a pulse

        IBI = sampleCounter - lastBeatTime;         // measure time between beats in mS
      lastBeatTime = sampleCounter;               // keep track of time for next pulse

      if(secondBeat){                        // if this is the second beat, if secondBeat == TRUE
        secondBeat = false;                  // clear secondBeat flag
        for(int i=0; i<=9; i++){             // seed the running total to get a realisitic BPM at startup
          rate[i] = IBI;                      
        }
      }

      if(firstBeat){                         // if it's the first time we found a beat, if firstBeat == TRUE
        firstBeat = false;                   // clear firstBeat flag
        secondBeat = true;                   // set the second beat flag
        sei();                               // enable interrupts again
        return;                              // IBI value is unreliable so discard it
      }   


      // keep a running total of the last 10 IBI values
      word runningTotal = 0;                  // clear the runningTotal variable    

      for(int i=0; i<=8; i++){                // shift data in the rate array
        rate[i] = rate[i+1];                  // and drop the oldest IBI value 
        runningTotal += rate[i];              // add up the 9 oldest IBI values
      }

      rate[9] = IBI;                          // add the latest IBI to the rate array
      runningTotal += rate[9];                // add the latest IBI to runningTotal
      runningTotal /= 10;                     // average the last 10 IBI values 
      BPM = 60000/runningTotal;               // how many beats can fit into a minute? that's BPM!
      QS = true;                              // set Quantified Self flag 
      // QS FLAG IS NOT CLEARED INSIDE THIS ISR
    }                       
  }

  if (Signal < thresh && pulso == true){   // when the values are going down, the beat is over

    pulso = false;                         // reset the Pulse flag so we can do it again
    amp = P - T;                           // get amplitude of the pulse wave
    thresh = amp/2 + T;                    // set thresh at 50% of the amplitude
    P = thresh;                            // reset these for next time
    T = thresh;
  }

  if (N > 2500){                           // if 2.5 seconds go by without a beat
    thresh = 512;                          // set thresh default
    P = 512;                               // set P default
    T = 512;                               // set T default
    lastBeatTime = sampleCounter;          // bring the lastBeatTime up to date        
    firstBeat = true;                      // set these to avoid noise
    secondBeat = false;                    // when we get the heartbeat back
  }

  sei();                                   // enable interrupts when youre done!
}// end isr

//------------------------------------------------------------------------------------------------------------------------
                                                                        
void setup() {
  pinMode(13,OUTPUT);                            //Se declara el puerto **** como salida
  lcd.init();                                   //Se inicializa la LCD 16x2
  lcd.backlight();                              
  lcd.clear();
  Serial.begin(9600);                           //Se configura el puerto a 9600 Baudios
  interruptSetup();                             //Se configura la interrupcion para leer el sensor de pulsos cada 2ms
}

void loop() {
  // put your main code here, to run repeatedly:
int pulsometro = analogRead (A0);               //Leer el valor del pulsometro conectado al puerto analogico A0
if (pulsometro >=530)                           //Enciende led del pin 13 cuando el pulso pasa de un valor *(se ajusta)*
  {
    digitalWrite(13, HIGH);
  }
else
  {
    digitalWrite(13,LOW);
  }
                                  //*Datos que se mostraran en LCD
  lcd.setCursor(1,0);                           //Se mostrara el valor de BPM en la LCD, en fila 1 - columna 0
  lcd.print("BPM= ");                           //Texto a mostrar
  lcd.print(BPM);                               //Se muestra el valor de BPM
  lcd.print("     ");
                                  //*Datos que se mostraran en el puerto serial
Serial.println(pulso);                         //Envia el valor del pulso por el puerto serie
if (QS == true)                                //Será verdadero cuando el arduino busca un pulso del corazón
  {
   QS = false;                                 //Reset 
  }
}
