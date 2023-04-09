1. Fix timers, ADC uses timer0A


2. Have flag that determines wether to calculate Jitter or Not
3. Improve updateing sleeping
4. Remove sleeping threads from priority

uint8_t timer1A_work = 0;
extern int32_t MaxJitter;             // largest time jitter between interrupts in usec
extern uint32_t const JitterSize;
extern uint32_t JitterHistogram[];
extern int32_t MaxJitter2;             // largest time jitter between interrupts in usec
extern uint32_t const JitterSize2;
extern uint32_t JitterHistogram2[];

void Timer1A_Handler(void){
  TIMER1_ICR_R = TIMER_ICR_TATOCINT;// acknowledge TIMER1A timeout
  long jitter;                    // time between measured and expected, in us

  if(timer1A_work++==0) return;

  unsigned static long LastTime;  // time at previous ADC sample
  uint32_t thisTime;              // time at current ADC sample
  long jitter;                    // time between measured and expected, in us

  thisTime = OS_Time();       // current time, 12.5 ns
  (*PeriodicTask1)();               // execute user task
  uint32_t diff = OS_TimeDifference(LastTime,thisTime);
  if(diff>(TIMER1_TAILR_R + 1 ) ){
    jitter = (diff-(TIMER1_TAILR_R + 1 ) +4)/8;  // in 0.1 usec
  }else{
    jitter = ((TIMER1_TAILR_R + 1 ) -diff+4)/8;  // in 0.1 usec
  }
  if(jitter > MaxJitter){
    MaxJitter = jitter; // in usec
  }       // jitter should be 0
  if(jitter >= JitterSize){
    jitter = JitterSize-1;
  }
  JitterHistogram[jitter]++; 
  LastTime = thisTime;

}