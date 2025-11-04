#include "mbed.h"
#include "TextLCD.h"
#include <string.h>
#include <stdbool.h>

Ticker flipper;
TextLCD lcd(p15, p16, p17, p18, p19, p20); //rs, e, d4-d7
SPI sw(p5, p6, p7);
DigitalOut lat(p8);
typedef enum { OFF, RED, GREEN, ORANGE } Colour;
//read from switches and keypad
BusOut cols_out(p26, p25, p24); 
BusIn rows_in(p14, p13, p12, p11);

DigitalOut Alarm_LED(LED1);// Alarm LED indicator

// Define the system state
enum AlarmState{ UNSET, EXIT, SET, ENTRY, ALARM, REPORT};
const char *names[] = {"UNSET", "EXIT", "SET", "ENTRY", "ALARM", "REPORT"};

AlarmState state = UNSET;// Initial system state

// numbers for password management
bool firstInput = true;// Flag to determine if this is the first password input
char storedCode[5] = {0};// Stores the user's password (4 digits + '\0')
char enteredCode[5] = {0}; // Stores user input for password verification
bool isCodeSet = false;// Flag to indicate whether a password has been set
Timeout alarmTimeout; // Timer to turn off the LED
// DigitalOut Alarm_LED(LED1); // Assuming LED1 is connected to Alarm_LED

// Keypad mapping for input detection
char Keytable[][4] = {
      {'1', '2', '3', 'F'},
      {'4', '5', '6', 'E'},
      {'7', '8', '9', 'D'},
      {'A', '0', 'B', 'C'}
    };
 
// read switch 
int read_switches(){
    cols_out = 0b100;// Activate first column of switches
    int left_4_switches = rows_in;// Read left 4 switches

    cols_out = 0b101;// Activate second column of switches
    int right_4_switches = rows_in;// Read right 4 switches
    // Combine left and right switch values
    int switches = (left_4_switches << 4) + right_4_switches;
    return switches;
}


void switch_leds(int switches, Colour colour) {
    int led_bits = 0;
    // for each switch value (in reverse order)
    for (int i=7; i >= 0; i--) {
        // there are 2 LEDs for each switch (GREEN and RED)
        // shift all led_bits 2 positions to the left
        led_bits <<= 2;
        // if switch is turned on, fill the new 2 bits with 
        // bits corresponding to the chosen colour
        if (switches & (1 << i)) {
            // set 2nd bit to 1 if GREEN should be on
            if (colour & 0b10) {
                led_bits |= 0b10;
            }
            // set 1st bit to 1 if RED should be on
            if (colour & 1) {
                led_bits |= 1;
            }            
        }
    }
    sw.write(led_bits);
    lat = 1;// Latch data to output
    lat = 0;
}



// detect key press (blocking)
char getKey(bool  =true, bool wait_until_released=true) {  
    for (int i = 0; i <= 3; i++) {
      cols_out = i;
      // for each bit in rows
       for (int j = 0; j <= 3; j++) {
           // if j'th bit of "rows_in" is LOW (indicating pressed key)
           if (~rows_in & (1 << j)) {    
               // until a key is released before returning it.
                return Keytable[j][3-i];
                                        
            }
        }
    }
    return ' ';
}
// detect key press (non-blocking) with timeout
char getKeyNonBlocking(unsigned int timeout_ms = 10) {
    Timer timer;
    timer.start();
    char key = ' ';
    while (timer.read_ms() < timeout_ms) {
        key = getKey();
        if (key != ' ') {   
            return key; // Small delay for better CPU efficiency
        }
        wait_us(1000); 
    }
    return ' ';// Return empty if no key detected within timeout
}




//wait until the user presses the 'B' button
void WaitBPress(){
    lcd.locate(0, 0);
    // 16 spaces to overwrite the first line
    lcd.printf("                ");  
    lcd.locate(0, 0);
    lcd.printf("Press B Button");
    while(true){
       // char key = getKey();
        if(getKey() == 'B'){
            break;
        }
        wait_us(250000);
    }
}






// set or enter a password
void setCode(char *code, bool maskInput, int required_code_size = 4) {
    int size = 0;
    // if maskInput = firstime, LCD shows "Set Code:____" otherwise shows "Enter Code:____"
    const char *prompt = maskInput ? "Set Code:____" : "Enter Code:____";
    int promptLength = strlen(maskInput ? "Set Code:" : "Enter Code:");
    lcd.locate(0, 1);
    lcd.printf("%s", prompt);

    while (size < required_code_size) {
        
        char key = getKeyNonBlocking(20);

        if (key == ' ') {
            // when SEXIT state, detect switches
            if (state == EXIT) {
                int switches = read_switches();
                if (switches >= 1) {
                    state = ALARM;  // trigger switches, state = alarm state
                    break;
                }
            }
            continue;
        }
         

        if (key == 'C' && size > 0) {
            size--;
            code[size] = '\0';
            lcd.locate(promptLength + size, 1);  // Adjust position based on the prompt length
            lcd.putc('_');
        } else if (key >= '0' && key <= '9') {
            lcd.locate(promptLength + size, 1);  // Adjust position based on the prompt length
            lcd.putc(maskInput ? key : '*');
            code[size] = key;
            size++;
        }
        wait_us(150000); //debounce
    }

    code[required_code_size] = '\0';
    wait_us(1000000);  // 1s delay
}



//for checkCode
bool checkCode(const char* storedCode, bool maskInput) {
    int code_size = 4;
    int maxAttempts = 3;// 3 times Attempts
    for (int attempt = 0; attempt < maxAttempts; attempt++) {
        lcd.cls();
        lcd.locate(0, 0);
        lcd.printf("%s wrong:%d", names[state],attempt);//lcd.printf("Attempt %d/%d", attempt, maxAttempts);
        
        setCode(enteredCode, maskInput, code_size); // cell setCode read for user input

        WaitBPress(); //wait B press
        
        // compare the input code and set are same
        if (strcmp(enteredCode, storedCode) == 0) {
            lcd.cls();
            lcd.locate(0, 0);
            lcd.printf("Correct Code!");
            wait_us(1000000);
            return true; // input Correct return true
        } else {
            lcd.cls();
            lcd.locate(0, 0);
            lcd.printf("Incorrect Code!");
            wait_us(1000000);
        }


    }
    return false; // 3 wrongs，return false
}

// LED blinking function
void flip()
{
    Alarm_LED = !Alarm_LED;
}

// Turn off the alarm
void turnOffAlarm() {
    Alarm_LED = 0; // Turn off alarm after 2 minutes
}

// Main program
int main() {
    int counterwrong;
    int counter_for_wrongcode = 0;
    char my_code[5] = {0};
    bool correctEntered = false;
    
    while (true){
        Timer t;
        lcd.cls();
        lcd.locate(0, 0);
        int switches = read_switches();
            switch(state){
                case UNSET:{
                    // lcd.printf("UNSET Mode");
                    lcd.printf("%s", names[state]);
                    switch_leds(switches, OFF);
                    if (!isCodeSet) {
                        setCode(storedCode, firstInput, 4); //save number to storedCode
                        
                        WaitBPress(); // Wait buttom B

                        lcd.cls();
                        lcd.locate(0, 0);
                        lcd.printf("success set code");
                        wait_us(1000000);
                        isCodeSet = true;
                        firstInput = false;
               
                    }
            
                    // 2. check the input is same as storedCode
                    if (checkCode(storedCode, firstInput)) {
                        state = EXIT;
                    } else {
                        lcd.cls();
                        lcd.locate(0, 0);
                        lcd.printf("3 wrong attempts");
                        wait_us(1000000);
                        state = ALARM;
                    }    
                    
                    break;
                }
                case EXIT:{
                    lcd.cls();
                    lcd.locate(0, 0);
                    lcd.printf("%s", names[state]);
                    lcd.locate(0,1);
                    lcd.printf("Enter Code:____");
                    wait_us(1000000);
                    bool actionOccurred = false;   // detect Keypad if input
                    bool switchTriggered = false;  // detect switches ON

                    Alarm_LED = 1;
                    flipper.attach(&flip, 0.5); // the address of the function to be attached (flip)
                    flip();

                    t.start();
                    int switches = 0;
            
                    sw.format(16,0);
                    sw.frequency(1000000);
                    lat = 0;
                    
                    //what for 60 min
                    while(t.read() < 60.0) {
                        
                        switches = read_switches();   
                        if(switches>= 1){
                           
                           switch_leds(switches, RED); // can choose different colours if needed
                           state = ALARM;
                           flipper.detach ();
                           switchTriggered = true;
                           break;
                        }

                        char key = getKeyNonBlocking(10);
                        if (key != ' ') {  
                            actionOccurred = true;
                            break;
                        }
                        wait_us(50000); // check evey 50ms


                    }
                    t.stop(); //time out

                    if (switchTriggered) {
                        break; // already into ALARM state，end EXIT
                    }


                    if (!actionOccurred) { // after 60 second no keypad input, go to SET
                        state = SET;
                        break;
                    }

                    if (checkCode(storedCode, firstInput)) {//enter code
                            lcd.printf("%s", names[state]);
                            Alarm_LED = 0;
                            flipper.detach ();
                            state = UNSET;
                    } else {
                        lcd.cls();
                        lcd.locate(0, 0);
                        lcd.printf("3 wrong attempts");
                        wait_us(1000000);
                        state = ALARM;
                    }
                        break;
                }
                
                case SET:{
                    lcd.cls();
                    lcd.locate(0, 0);
                    lcd.printf("%s", names[state]);

                    
                    t.start();
                    bool sensorTriggered = false;

                    // what for 60 second, is checking switches
                    while (t.read() < 60.0 && !sensorTriggered) {
                            int switches = read_switches(); // read switches 

                            // if switches & 0x7F，go to ALARM 
                            if ((switches & 0x7F) != 0) {
                                state = ALARM;
                                sensorTriggered = true;
                            break;
                            }
                            //if highest switches & 0x80, go to ENTRY
                            if ((switches & 0x80) != 0) {
                                state = ENTRY;
                                sensorTriggered = true;
                                break;
                            }

                            wait_us(50000); 
                    }

                    t.stop();
                    break;
                }
                case ENTRY:{ 
                    lcd.cls();
                    lcd.locate(0, 0);
                    lcd.printf("%s", names[state]);
                    lcd.locate(0, 1);
                    lcd.printf("Enter Code:____");
                    
                    
                    t.start();
                    bool codeEntered = false;
                    bool sensorTriggered = false;

                    // LED flash
                    Alarm_LED = 1;
                    flipper.attach(&flip, 0.5);

                    // // Entry period：2 min
                    while (t.read() < 120.0) {
                       // check any key input
                        char key = getKeyNonBlocking(10);
                        if (key == ' ') {
                            // if no key input
                            int switches = read_switches();
                            if ((switches & 0x7F) != 0) {
                                sensorTriggered = true;
                                state = ALARM;
                                break;
                            }
                        }else{
                            // if have key input, enter code
                            setCode(storedCode, firstInput, 4);
                    
                            WaitBPress();
                            if (strcmp(enteredCode, storedCode) == 0) {
                                codeEntered = true;
                                state = UNSET;
                                break;
                            } else {
                            wait_us(1000000);
                            //allow input code during time
                            }
                             wait_us(50000); //check 50ms
                        }
                    }
                       
                    t.stop();
                    flipper.detach(); // stop LED flash

                    // if entry period don't put correct, and no switchs triggeres then go to ALARM
                    if (!codeEntered && !sensorTriggered) {
                         state = ALARM;
                    }
                    break;
                }


                case ALARM:{

                    Alarm_LED = 1;
                    flipper.detach ();
                    lcd.locate(0,1);
                    lcd.printf("Enter Code:____");
                    wait_us(1000000);
                    alarmTimeout.attach(&turnOffAlarm, 120.0); // 120 seconds (2 minutes)
                    if (checkCode(storedCode, firstInput)) {
                        state = REPORT;
                    }
                    break;
                }
                case REPORT:
                    int switches = read_switches();
                    char key = getKeyNonBlocking();
                    int switches_entry = switches & 0x7F;
                    lcd.cls();
                    lcd.locate(0, 0);
                    lcd.printf("code error %d", switches_entry);
                    lcd.locate(0, 1);
                    lcd.printf("C key to clear");
                    wait_us(1000000);
                    if (key == 'C' ) {
                        state = UNSET;
                        firstInput = true;
                    }
                    break;
            }
        
    }
}
