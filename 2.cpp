#include <Servo.h>
Servo pan;

/* ===== pins (arduino uno) ===== */
// l298n (اتجاهات مصحّحة)
#define ENA  5   // pwm right  (ارفع jumper ena)
#define ENB  6   // pwm left   (ارفع jumper enb)
#define IN1  3   // right
#define IN2  4
#define IN3  2   // left
#define IN4  7

// ultrasonic
const int TRIG = 12;
const int ECHO = 13;

// servo
const int SERVO_PIN = A0;

// ir sensors (lm393 غالبًا active low)
const int IR_R  = 8;   // يمين خارجي
const int IR_RM = 9;   // يمين داخلي
const int IR_LM = 10;  // يسار داخلي
const int IR_L  = 11;  // يسار خارجي
const bool IR_ACTIVE_LOW = true;  // لو حساساتك بالعكس خلّيها false

/* ===== tracking params ===== */
int minFollow = 30;     // أدنى مسافة مريحة
int maxFollow = 50;     // أقصى مسافة مريحة
int backLimit = 10;     // أقرب من هيك -> رجوع
int farLimit  = 150;    // أبعد مدى نعتبره هدف

int carSpeed   = 190;   // سرعة التتبّع
int turnSpeed  = 200;   // سرعة الدوران

/* ===== servo (fast but smooth on acquire only) ===== */
int centerAngle = 90;   // لو وسطك الحقيقي 100 غيّرها
int rightAngle  = 30;
int leftAngle   = 150;
int stepDeg     = 6;    // خطوة أكبر = أسرع
int stepDelayMs = 8;    // تأخير أقل = أسرع
int settleMs    = 100;  // مهلة قصيرة قبل القياس

/* ===== reacquire policy ===== */
unsigned long reacquireEveryMs = 3000; // إعادة اكتساب كل 3 ثواني حدًا أقصى
unsigned long lostTimeoutMs    = 1000; // فقد الهدف 1 ثانية -> reacquire

/* ===== state ===== */
enum Mode { ACQUIRE, FOLLOW };
Mode mode = ACQUIRE;
unsigned long lastSeenMs = 0, lastAcquireMs = 0;

/* ===== motor helpers (corrected) ===== */
void stopAll(){
  analogWrite(ENA,0); analogWrite(ENB,0);
  digitalWrite(IN1,LOW); digitalWrite(IN2,LOW);
  digitalWrite(IN3,LOW); digitalWrite(IN4,LOW);
}
void forward(){
  analogWrite(ENA,carSpeed); analogWrite(ENB,carSpeed);
  digitalWrite(IN1,LOW);  digitalWrite(IN2,HIGH);  // right fwd
  digitalWrite(IN3,LOW);  digitalWrite(IN4,HIGH);  // left  fwd
}
void back(){
  analogWrite(ENA,carSpeed); analogWrite(ENB,carSpeed);
  digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW);   // right back
  digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW);   // left  back
}
void turnRight(){
  analogWrite(ENA,turnSpeed); analogWrite(ENB,turnSpeed);
  digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW);   // right back
  digitalWrite(IN3,LOW);  digitalWrite(IN4,HIGH);  // left  fwd
}
void turnLeft(){
  analogWrite(ENA,turnSpeed); analogWrite(ENB,turnSpeed);
  digitalWrite(IN1,LOW);  digitalWrite(IN2,HIGH);  // right fwd
  digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW);   // left  back
}

/* ===== ultrasonic ===== */
long distOnce(){
  digitalWrite(TRIG,LOW); delayMicroseconds(2);
  digitalWrite(TRIG,HIGH); delayMicroseconds(10);
  digitalWrite(TRIG,LOW);
  long du = pulseIn(ECHO,HIGH,30000UL);
  if(!du) return 400;
  return (long)(du*0.0343/2.0);
}
// median of 3 لثبات القراءة
long distCM(){
  long a=distOnce(); delay(2);
  long b=distOnce(); delay(2);
  long c=distOnce();
  if(a>b){long t=a;a=b;b=t;} if(b>c){long t=b;b=c;c=t;} if(a>b){long t=a;a=b;b=t;}
  return b;
}

/* ===== servo smooth (acquire only) ===== */
void goSmooth(int target){
  pan.attach(SERVO_PIN,700,2400);
  int cur = pan.read(); if(cur<0||cur>180) cur=centerAngle;
  int s = (target>cur)? stepDeg : -stepDeg;
  while(cur!=target){
    cur += s;
    if((s>0 && cur>target)||(s<0 && cur<target)) cur=target;
    pan.write(cur); delay(stepDelayMs);
  }
  delay(settleMs);
}
void centerDetach(){ goSmooth(centerAngle); pan.detach(); }

/* ===== ir helpers ===== */
bool irActive(int pin){
  int v = digitalRead(pin);
  return IR_ACTIVE_LOW ? (v==LOW) : (v==HIGH);
}
bool anyObstacle(){
  return irActive(IR_R) || irActive(IR_RM) || irActive(IR_LM) || irActive(IR_L);
}

/* تفادي سريع بالـ ir (أولوية أعلى من التتبّع) */
void avoidByIR(){
  bool r  = irActive(IR_R)  || irActive(IR_RM);
  bool l  = irActive(IR_L)  || irActive(IR_LM);

  stopAll(); delay(30);

  if (r && l){
    back();      delay(250);
    stopAll();   delay(50);
    turnRight(); delay(200);   // لفّة صغيرة لتغيير اتجاه
    stopAll();   delay(40);
    return;
  }
  if (r){
    back();      delay(120);
    stopAll();   delay(30);
    turnLeft();  delay(200);
    stopAll();   delay(40);
    return;
  }
  if (l){
    back();      delay(120);
    stopAll();   delay(30);
    turnRight(); delay(200);
    stopAll();   delay(40);
    return;
  }
}

/* ===== acquire target once ===== */
bool acquire(){
  long bestD=9999; int bestA=centerAngle;

  goSmooth(rightAngle);
  long dR=distCM(); if(dR>5 && dR<bestD){bestD=dR; bestA=rightAngle;}

  goSmooth(centerAngle);
  long dC=distCM(); if(dC>5 && dC<bestD){bestD=dC; bestA=centerAngle;}

  goSmooth(leftAngle);
  long dL=distCM(); if(dL>5 && dL<bestD){bestD=dL; bestA=leftAngle;}

  centerDetach();

  if(bestD>=farLimit || bestD==400) return false;

  if(bestA==rightAngle){ turnRight(); delay(200); stopAll(); }
  else if(bestA==leftAngle){ turnLeft(); delay(200); stopAll(); }

  lastSeenMs = millis();
  return true;
}

/* ===== follow using front only (servo detached) ===== */
void follow(){
  long d = distCM();
  if(d>0 && d<400) lastSeenMs = millis();

  // قريب جدًا
  if(d>0 && d<backLimit){
    stopAll(); delay(10);
    back();   delay(150);
    stopAll(); delay(20);
    return;
  }

  unsigned long now = millis();
  if(d>=farLimit || (now-lastSeenMs)>lostTimeoutMs || (now-lastAcquireMs)>reacquireEveryMs){
    mode = ACQUIRE;
    return;
  }

  if(d > maxFollow){
    forward();  delay(140);
    stopAll();  delay(20);
  } else if(d < minFollow && d>backLimit){
    back();     delay(100);
    stopAll();  delay(20);
  } else {
    stopAll();  delay(20);
  }
}

/* ===== setup ===== */
void setup(){
  Serial.begin(9600);

  pinMode(IN1,OUTPUT); pinMode(IN2,OUTPUT);
  pinMode(IN3,OUTPUT); pinMode(IN4,OUTPUT);
  pinMode(ENA,OUTPUT); pinMode(ENB,OUTPUT);

  pinMode(TRIG,OUTPUT); pinMode(ECHO,INPUT);

  pinMode(IR_R,INPUT); pinMode(IR_RM,INPUT);
  pinMode(IR_LM,INPUT); pinMode(IR_L,INPUT);

  pan.attach(SERVO_PIN,700,2400);
  pan.write(centerAngle);
  delay(200);
  stopAll();

  mode = ACQUIRE; lastAcquireMs = 0; lastSeenMs = millis();
}

/* ===== loop ===== */
void loop(){
  // أولوية: تفادي عوائق ir
  if (anyObstacle()){
    avoidByIR();
    return;
  }

  // تتبّع سريع
  if(mode==ACQUIRE){
    if(acquire()){ mode=FOLLOW; lastAcquireMs=millis(); }
    else { turnRight(); delay(150); stopAll(); delay(40); }
    return;
  }
  follow();
}
