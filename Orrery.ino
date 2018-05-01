#define MAX_STEP (2960)
#define STEP_DELAY (150)

#define CHECK_BIT(var,pos) ((var) & (1<<(pos))) // Checks if bit is 1
#define TO_RADIAN(A) (PI * (A) / 180)
#define TO_DEGREE(R) ((R) * (180/PI))
#define POS_TO_STEP(P) (P * MAX_STEP)


#define ENABLE (8)  

//Direction pin
#define MRC_DIR	(5)
#define VNS_DIR	(6)
#define ERT_DIR	(13)
#define MRS_DIR	(7)

//Step pin
#define MRC_STP	(2)
#define VNS_STP	(3)
#define ERT_STP	(12)
#define MRS_STP	(4) 

#define MERCURY (0)
#define VENUS (1)
#define EARTH (2)
#define MARS (3)

#define LEFT (true)
#define RIGHT (!LEFT)

#define HYBRID

// A date has day 'd', month 'm' and year 'y'
struct Date {
    int d, m, y;
};

byte directionPins[4] = {MRC_DIR, VNS_DIR, ERT_DIR, MRS_DIR};
byte stepPins[4] = {MRC_STP, VNS_STP, ERT_STP, MRS_STP};
byte stepPatterns[4] = { B11111111, B10100100, B10001000, B10000000};

int stepPositions[4] = {0, 0, 0, 0}; 
float floatPositions[4] = {0, 0, 0, 0}; 
float simSpeed = 0.0001;

Date inputDate = {.d = 0, .m = 0, .y = 0};
boolean inputComplete = false;  // whether the input is complete

void setup(){
    for(int i = 0; i < 4; i++){
		pinMode(directionPins[i], OUTPUT);
	}

	for(int i = 0; i < 4; i++){
		pinMode(stepPins[i], OUTPUT);
	}
    pinMode(ENABLE, OUTPUT);
	initAnimation();
    Serial.begin(9600);
    
    digitalWrite(ENABLE, HIGH);
    delay(5000);
    digitalWrite(ENABLE, LOW);
    delay(500);
}

void loop(){

    #ifdef HYBRID
	    if (inputComplete) {
  			Serial.println(inputDate.y);
  			Serial.println(inputDate.m);
  			Serial.println(inputDate.d);
  
  			Date targetDate = {.d = 1, .m = 11, .y = 2808};
  			setPositionByDate(inputDate);
  			stepToCurrentPosition();
  			delay(5000);
  			inputDate = {.d = 0, .m = 0, .y = 0};
  			inputComplete = false;
  			initAnimation();
		  }
    	animationStep();
      delay(10);
		  //delayMicroseconds(1000);
    #endif
	//delay(100);
}

void serialEvent() {
	int inputYear = Serial.parseInt();
	int inputMonth = Serial.parseInt();
	int inputDay = Serial.parseInt();

	inputDate = {.d = inputDay, .m = inputMonth, .y = inputYear};
	while (Serial.available()) {
		if ((char)Serial.read() == '\n') {
			inputComplete = true;
		}
	}
}

//float referenceLoactions[4] = {0.491778, 0.2945, 0.0664444, 0.7875};	// Solar lon (0-1) of 01/01/2000
int referenceLoactions[4] = {4917, 2945,  664, 7875};	// Solar lon (0-10000) of 01/01/2000
float orbitalSpeeds[4] = {113.636, 44.5038, 27.3823, 14.556};
Date referenceDate = {.d = 1, .m = 1, .y = 2000};
void setPositionByDate(Date d){
	long days = getDifference(referenceDate, d);
	for(int planet = 0; planet < 4; planet++){
		long newPosition = (referenceLoactions[planet] + (long)(days * orbitalSpeeds[planet])) % 10000;	
		floatPositions[planet] = newPosition / 10000.0;
    }
}

// To store number of days in all months from January to Dec.
const int monthDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
 
// This function counts number of leap years before the
// given date
int countLeapYears(Date d) {
    int years = d.y;
    // Check if the current year needs to be considered
    // for the count of leap years or not
    if (d.m <= 2){
        years--;
    }
    // An year is a leap year if it is a multiple of 4,
    // multiple of 400 and not a multiple of 100.
    return years / 4 - years / 100 + years / 400;
}
 
// This function returns number of days between two given
// dates
int getDifference(Date dt1, Date dt2) {
    // COUNT TOTAL NUMBER OF DAYS BEFORE FIRST DATE 'dt1'
    // initialize count using years and day
    long int n1 = dt1.y*365 + dt1.d;
    // Add days for months in given date
    for (int i=0; i<dt1.m - 1; i++){
        n1 += monthDays[i];
    }
    // Since every leap year is of 366 days,
    // Add a day for every leap year
    n1 += countLeapYears(dt1);
 
    // SIMILARLY, COUNT TOTAL NUMBER OF DAYS BEFORE 'dt2'
    long int n2 = dt2.y*365 + dt2.d;
    for (int i=0; i<dt2.m - 1; i++){
        n2 += monthDays[i];
    }
    n2 += countLeapYears(dt2);
    // return difference between two counts
    return (n2 - n1);
}

byte currentAnimationStep = 0;
void animationStep(){
    for(int planet = 0; planet < 4; planet++){
        if(CHECK_BIT(stepPatterns[planet], currentAnimationStep)){
        	stepPositions[planet] = (stepPositions[planet] + 1) % MAX_STEP;
            digitalWrite(stepPins[planet], HIGH);
        }
    }
    delayMicroseconds(STEP_DELAY);
    for(int planet = 0; planet < 4; planet++){
        digitalWrite(stepPins[planet], LOW);
    }
    delayMicroseconds(STEP_DELAY);
    currentAnimationStep = (currentAnimationStep + 1) % 8;
}

void initAnimation(){
	currentAnimationStep = 0;
    for(int planet = 0; planet < 4; planet++){
		digitalWrite(directionPins[planet], RIGHT);
	}
}

// calculate how to step to the given bearing, then step there
void stepToCurrentPosition() {
  currentAnimationStep = 0;
	int steps[4] = {0, 0, 0, 0};
	boolean dir[4] = {false, false, false, false};

    for(int planet = 0; planet < 4; planet++){
		int current_step = stepPositions[planet];
		int target_step = POS_TO_STEP(floatPositions[planet]);
		int distance_r, distance_l;
		// these formulas can probably be reduced
		if(target_step == current_step) {	// no need to move
			continue;
		} else if(target_step < current_step) {
			distance_r = (MAX_STEP - current_step) + target_step;
			distance_l = current_step - target_step;
		} else {
			distance_r = target_step - current_step;
			distance_l = current_step + (MAX_STEP - target_step);
		}
    	// call step function in correct direction
    	//dir[planet] = distance_l < distance_r;	// Find shortest direction
    	dir[planet] = distance_l > distance_r;	// Find longer direction
    	steps[planet] = (dir[planet] ? distance_l : distance_r);
    	
    }
    
    moveToPosition(steps, dir);
    
}


// step the motor
void moveToPosition(int steps[], boolean dir[]){
	int maxStep = -1;
	for(int planet = 0; planet < 4; planet++){
		if(steps[planet] > maxStep){
			maxStep = steps[planet];
		}
		digitalWrite(directionPins[planet], dir[planet]);
		stepPositions[planet] +=  dir[planet] ? -steps[planet] : steps[planet];
		if(stepPositions[planet] < 0){
			stepPositions[planet] = stepPositions[planet] + MAX_STEP;
		}
		stepPositions[planet] %= MAX_STEP;
	}
	
	for(int i = 0; i < maxStep; ++i) {
		for(int planet = 0; planet < 4; planet++){
			if(i < steps[planet]){
				digitalWrite(stepPins[planet], HIGH); 
			}
		}
		delayMicroseconds(STEP_DELAY); 
		for(int planet = 0; planet < 4; planet++){
			if(i < steps[planet]){
				digitalWrite(stepPins[planet], LOW); 
			}
		}
		delayMicroseconds(STEP_DELAY); 
		delay(1);
	}
}
