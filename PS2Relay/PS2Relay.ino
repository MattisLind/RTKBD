/*
 Test code to set a PS/2 keyboard into Code set 3
*/

#define HOSTDATA PB12
#define HOSTCLK  PB13
#define KBDCLK  PB7
#define KBDDATA PB6
#define SHORT 20
#define LONG 40


void inline gohi (int pin) {
  pinMode(pin, INPUT_PULLUP);
  digitalWrite(pin, HIGH);
}

void inline golo (int pin) {
  digitalWrite(pin, LOW);
  pinMode(pin, OUTPUT);
}


int restore() {
//  Serial.print("Z");
  gohi(HOSTCLK);
  gohi(HOSTDATA);
  return -1;
}

int sendBits (int numBits, unsigned char data, int * numOnes) {
  delayMicroseconds(SHORT);
  do {
    if (data & 1) {
      gohi(HOSTDATA);
      (*numOnes)++;
    } else {
      golo(HOSTDATA);
    }
    data >>= 1;
    delayMicroseconds(SHORT);
    if (digitalRead(HOSTCLK)==0) {
//      Serial.print("Y");
      return numBits;
    }
    golo(HOSTCLK);
    delayMicroseconds(LONG);
    gohi(HOSTCLK);
    if (digitalRead(HOSTCLK)==0) {
//      Serial.print("X");
      return numBits;
    }
    numBits--;
  } while (numBits != 0);
  return numBits;
}

int sendNonWait (unsigned char data) {
  Serial.write(data);
  int numOnes = 0;
  if (sendBits(1, 0, &numOnes)==-1) return restore();
  if (sendBits(8, data, &numOnes)==-1) return restore();
  if (sendBits(1,~numOnes, &numOnes)==-1) return restore();
  sendBits(1, 1, &numOnes);
  return 0;
}

int send (unsigned char data) {
  int ret;
  //Serial.print("In Send:");
  //Serial.write(data);
  //Serial.write(0xC2);
  do {
    while (digitalRead(HOSTCLK)==0);
    if (digitalRead(HOSTDATA) == 0) return -1;   
    ret = sendNonWait(data);
    delayMicroseconds(40);
    //Serial.write(0xC4);
  } while (ret==-1);
  //Serial.write(0xC3);
  return ret;
}



unsigned char readBits(int numBits, int * numOnes) {
  unsigned char data=0;
  while (1) {
    golo(HOSTCLK);
    delayMicroseconds(LONG);
    gohi(HOSTCLK);
    delayMicroseconds(SHORT);
    if (digitalRead(HOSTDATA)==1) {
      (*numOnes)++;
      data |= 0x80;
    }   
    numBits--;
    if (numBits == 0) return data;
    data >>= 1;
    delayMicroseconds(SHORT);
  }
}


void sendAckBit () {
  golo(HOSTDATA);
  delayMicroseconds(SHORT);
  golo(HOSTCLK);
  delayMicroseconds(LONG);
  gohi(HOSTCLK);
  delayMicroseconds(SHORT);
  gohi(HOSTDATA);
}

int receiveNoWait () {
  int data, numOnes=0, parityAndStopbit, dummy=0, tmp=0;
  data = readBits(8, &numOnes);
  parityAndStopbit = readBits(2, &dummy);
  if ((parityAndStopbit & 0x80)==0) {
    do {
      tmp = readBits(1, &dummy);
    } while ((tmp & 0x80)==0);
    return -1;
  }
  if ((((parityAndStopbit & 0x40)==0) && ((numOnes & 1) == 1)) || (((parityAndStopbit & 0x40)==0x40) && ((numOnes & 1) == 0))) {
    sendAckBit();
    Serial.write(data);
    return data;
  } else {
    sendAckBit();
    return -1;
  }
}

int receive () {
  int ret;
  do {
    do {
      while (digitalRead(HOSTCLK) == 0);
    } while (digitalRead(HOSTDATA)==1);
    ret = receiveNoWait();
    if (ret!=-1) return ret;
    send(0xfe);
  } while (ret ==-1);
}







char txByte;
char txMode=0;
int cnt=0;
char busy;
unsigned char rxByte;
char parity=1; 
char rxDone;
char txState;
unsigned long lastFalling = micros();
void ISR () {
  char bit;/*
  unsigned long now = micros();
  if ((now-lastFalling) > 1000000) {
    cnt = 0;
    lastFalling=now;
  }*/
  if (txMode) {
    //if (cnt == 0) Serial.println(txByte,HEX);
    if (cnt >=0 && cnt <8) {
      bit = txByte & 0b00000001;
      txByte >>= 1;
      parity ^= bit;
      cnt++;
      if (bit) {
        gohi(KBDDATA);
      } else {
        golo(KBDDATA);
      }
    } else if (cnt==8) {
      if (parity) {
        gohi(KBDDATA);
      } else {
        golo(KBDDATA);
      }
      cnt++;
    } else if (cnt == 9) {
      gohi(KBDDATA);
      cnt++;
    } else if (cnt == 10 ) {
      txMode = 0;
      busy = 0;
      cnt = 0;
    }
  } else {
    bit = digitalRead(KBDDATA);
    if (cnt >0 && cnt <9) {
      parity ^= bit;
      cnt++;
      rxByte >>= 1;
      if (bit) rxByte |= 0b10000000;
    } else if (cnt==0) {
      parity=0;
      rxByte = 0;
      busy = 1;
      cnt++;  
    } else if (cnt==9) {
      parity  ^= 1;
      cnt++;
    } else if (cnt == 10) {
      cnt = 0;
      busy = 0;
      rxDone = 1;
      Serial.write(rxByte);
    }
  }
}


void setup() {
  afio_cfg_debug_ports(AFIO_DEBUG_SW_ONLY);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(HOSTDATA, INPUT_PULLUP);
  pinMode(HOSTCLK, INPUT_PULLUP);
  pinMode(KBDDATA, INPUT_PULLUP);
  pinMode(KBDCLK, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(KBDCLK),ISR,FALLING); 
  Serial.begin(9600);
}


void startTx (char byteToSend) {
  busy = 1;
  parity = 1;
  txByte = byteToSend;
  // Setting pin output low will cause interrupt before ready
  detachInterrupt( digitalPinToInterrupt( KBDCLK ) );
  // set pins to outputs and high
  gohi(KBDDATA);
  gohi(KBDCLK);
  // Essential for PS2 spec compliance
  delayMicroseconds( 10 );
  // set Clock LOW
  golo(KBDCLK);
  // Essential for PS2 spec compliance
  // set clock low for 60us
  delayMicroseconds( 60 );
  // Set data low - Start bit
  golo(KBDDATA);
  delayMicroseconds( 10 );
  // set clock to input_pullup data stays output while writing to keyboard
  gohi(KBDCLK);
  txMode = 1;
  // Restart interrupt handler
  attachInterrupt( digitalPinToInterrupt( KBDCLK ), ISR, FALLING );
  //  wait clock interrupt to send data
}

char firstByte;
char secondByte;

void processSendFSM () {
  if (txState==1) {
    //Serial.println("txState=1");
    startTx(firstByte);
    rxDone = 0;
    txState=2;
  } else if (txState == 2) { 
    
    if (rxDone==1) {
      //Serial.println("txState=2");
      //Serial.println(rxByte,HEX);
      rxDone = 0;
      if (rxByte ==0xfa) {
        startTx(secondByte);
        txState = 0;
      } else if (rxByte == 0xfe) {
        txState = 1;  
      }
    } 
  }  
}

void sendSetActionMake() {
 txState = 1;
 secondByte = 0x58;
 firstByte = 0xFC; 
}


void sendSetScanCodeSet(char scancodeset) {
 txState = 1;
 secondByte = scancodeset;
 firstByte = 0xF0; 
}

void sendSetResetLEDs(char ledState) {
   txState = 1;
   secondByte = ledState;
   firstByte = 0xED;  
}
char leds=0;

char lastSent;

int kbdState = 8;

unsigned long startup = micros();

void handleKeyboard () {
  //Serial.write(0xff);
  if (kbdState == 8 && (micros()-startup) > 3000000) {
    kbdState = 3;
  }
  if (kbdState == 3) {
    //Serial.println("kbdState=3");  
    startTx(0xff);
    kbdState = 4;  
  }
  if (kbdState == 4 && txState == 0 && rxDone == 1) {
    if (rxByte == 0xfa) kbdState = 0;
    else kbdState = 3;
    //Serial.println("kbdState=4"); 
    rxDone = 0;
  }
  if (rxDone == 1 && kbdState == 0 && rxByte == 0xaa) {
    //Serial.println("kbdState=0");
    //Serial.println(rxByte,HEX);
    rxDone = 0;
    sendSetScanCodeSet(0x03); 
    kbdState = 1; 
  }
  if (txState == 0 && kbdState == 1 && rxDone == 1) {
    rxDone = 0;
    //Serial.println("kbdState=1");
    //Serial.println(rxByte,HEX);
    /*if (rxByte == 0xaa) {
      sendSetScanCodeSet(0x03); 
      kbdState = 1;    
    }*/
    startTx(0xfa);
    kbdState = 2;
  }
  if (txState == 0 && kbdState == 2 && rxDone == 1) {
    rxDone = 0;
    //Serial.println("kbdState=2");
    //Serial.println(rxByte,HEX);
    sendSetActionMake();
    kbdState = 6;
  }
  if (txState == 0 && kbdState==6 && rxDone == 1) {
    rxDone = 0;
    //Serial.println("kbdState=6");
    //Serial.println(rxByte,HEX);
    startTx(0xf4);
    kbdState = 7;
  }
  if (txState == 0 && kbdState == 7 && rxDone == 1) {
    rxDone = 0;
    //Serial.println(micros(), DEC);
    //Serial.println("kbdState=7");
    //Serial.println(rxByte,HEX);
    if (rxByte == 0xaa) {
      sendSetScanCodeSet(0x03); 
      kbdState = 1;    
    }
    if (rxByte < 0xfa) {
      //Serial.write(rxByte); 
      send(rxByte);   
    }
  }
}

#define INTERBYTETIME 2000 // 2 ms 

// the loop function runs over and over again forever
void loop() {
  int cnt;
  int data;
  int command;
  char inputChar;
  int resetWaitCounter;
  unsigned long then;
  //goto loop;
  delay(300); // 300 ms startup delay
  while (digitalRead(HOSTCLK)==0);
  if (digitalRead(HOSTDATA)==0) {
    while (1) {
      command = receive();      
      if (command != -1) break;
      send(0xfe);  // send a resend.
    }
    goto processCommand;
  } else {
    send (0xaa); // send power up selftest ok.
  }
  then = micros();
  while ((micros() - then) < INTERBYTETIME) {
    if (digitalRead(HOSTCLK)==0) continue;
    if (digitalRead(HOSTDATA)==0) break;
  }
  if ((digitalRead(HOSTCLK)==0)) goto loop;
  else if (digitalRead(HOSTDATA)==0) {
    while (1) {
      command = receive();
      if (command != -1) break;
      send(0xfe);  // send a resend.
    } 
    goto processCommand;   
  } 
sendId:    
  send(0xbf);
  then = micros();
  while ((micros() - then) < INTERBYTETIME) {
    if (digitalRead(HOSTCLK)==0) continue;
    if (digitalRead(HOSTDATA)==0) break;
  }
  if ((digitalRead(HOSTCLK)==0)) goto loop;
  else if (digitalRead(HOSTDATA)==0) {
    while (1) {
      command = receive();    
      if (command != -1) break;
      send(0xfe);  // send a resend.
    } 
    goto processCommand;   
  }
  send(0xb1);
loop:
  //Serial.write(0xC0);
  while (digitalRead(HOSTCLK)==0);
  if (digitalRead(HOSTDATA)==0) {
    //Serial.write(0xFE);
    while (1) {
      command = receiveNoWait();
     
      if (command != -1) break;
      send(0xfe);  // send a resend.
    }
processCommand:
    switch (command) {
      case 0xff:
        delayMicroseconds(10);
        send(0xfa); 
        send (0xaa);
        resetWaitCounter = 62;
        then = micros();
wait:
        if (digitalRead(HOSTCLK)==0) {
          if ((micros() - then) > 500) goto loop; 
          else {
            resetWaitCounter = 62;
            goto wait;
          }
        } 
        if (digitalRead(HOSTDATA)==0) {
          receive();
          goto processCommand;
        } else {
          resetWaitCounter--;
          if (resetWaitCounter > 0) goto wait;
          return; // restart from the beginning! Doing the full reset.
        }
        
        break; 
      case 0xfe:
        send (lastSent);
        goto loop;
        break;
      case 0xee:
        send(0xee);
        goto loop;
      case 0xe6:
        send(0xfc);
        goto loop;
      case 0xfa:
      case 0xfb:
      case 0xfc:
      case 0xfd:
resendAck:
        send(0xfa);
        //Serial.write(0xA1);
        command = receive();
        //Serial.write(0xA2);
        if (command == 0xfe) goto resendAck;
        //Serial.write(0xA3);
        if ((command < 0xed) || command == 0xf0 || command == 0xef) {
          //Serial.write(0xA4);
          goto resendAck;  
        }
        //Serial.write(0xA5);
        goto processCommand;
      case 0xf9:
      case 0xf8:
      case 0xf7:
      case 0xf6:
      case 0xf5:
      case 0xf4:
        //Serial.write(0xC5);
        send(0xfa);
        //Serial.write(0xC1);
        goto loop;
      case 0xf3:
        send(0xfa);
        command = receive();
        if (command & 0x80) goto processCommand;
        else send(0xfa);
        goto loop;
      case 0xf2:
readIdResendAck:      
        send(0xfa);
        while (digitalRead(HOSTCLK)==0);
        if (digitalRead(HOSTDATA) == 0) {
          command = receive();
          if (command == 0xef) goto readIdResendAck;  
          goto processCommand;
        }
        goto sendId;
      case 0xed:
        send(0xfa);
        command = receive();
        if (command & 0x80 || command & 0x40 || command & 0x20 || command & 0x10) goto processCommand;
        send(0xfa);
        sendSetResetLEDs(command);
        goto loop;      
      default:
        send(0xfc);
        goto loop;  
    }    
  } else {
    //Serial.write(0xfd);
    processSendFSM(); 
    handleKeyboard(); 
  }
  goto loop;

}
