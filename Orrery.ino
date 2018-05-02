#define MAX_STEP (2960)		// Number of steps per full planet revolution
#define STEP_DELAY (150)	// Time in uS to wait inbetween steps

#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))	// Checks if bit `pos` in `var` is 1
#define TO_RADIAN(A) (PI * (A) / 180)	// Converts from degree to radian
#define TO_DEGREE(R) ((R) * (180/PI))	// Converts from radian to degree
#define POS_TO_STEP(P) (P * MAX_STEP)	// Converts a float from 0-1 to 0-MAX_STEP


#define ENABLE (8)	// Pin that enables or disables all motor control

// Direction pins
#define MRC_DIR	(5)
#define VNS_DIR	(6)
#define ERT_DIR	(13)
#define MRS_DIR	(7)

// Step pins
#define MRC_STP	(2)
#define VNS_STP	(3)
#define ERT_STP	(12)
#define MRS_STP	(4) 

// Enumerations for the planets, used primaraly as indexes for arrays
#define MERCURY (0)
#define VENUS (1)
#define EARTH (2)
#define MARS (3)

// Directions :D
#define LEFT (true)
#define RIGHT (!LEFT)

// Defines what mode to send to the arduino, currently only HYBRID exists. 
#define HYBRID

// A date has day 'd', month 'm' and year 'y'
struct Date {
    int d, m, y;
};

byte directionPins[4] = {MRC_DIR, VNS_DIR, ERT_DIR, MRS_DIR};
byte stepPins[4] = {MRC_STP, VNS_STP, ERT_STP, MRS_STP};

int stepPositions[4] = {0, 0, 0, 0};	// Current actual positons of the stepper motors, although this can become inaccurate over time with missed steps
float floatPositions[4] = {0, 0, 0, 0}; 	// Desired position to go to when stepToCurrentPosition() is called

// Variables for serial date input
Date inputDate = {.d = 0, .m = 0, .y = 0};
boolean inputComplete = false;  // whether the input is complete

void setup(){
	// Starts serial
	Serial.begin(9600);
	
	// Set all direction, step, and enable pins to output
    for(int i = 0; i < 4; i++){
		pinMode(directionPins[i], OUTPUT);
		pinMode(stepPins[i], OUTPUT);
	}
    pinMode(ENABLE, OUTPUT);

    // Setup animation
	initAnimation();
    
    // 5 second delay before starting animation to allow for manual alignment
    digitalWrite(ENABLE, HIGH);
    delay(5000);
    digitalWrite(ENABLE, LOW);
    delay(500);
}

void loop(){

    #ifdef HYBRID
    	// If there is a date input
		if (inputComplete) {
  			Serial.println(inputDate.y);
  			Serial.println(inputDate.m);
  			Serial.println(inputDate.d);

  			// calculate input date's position and snap to it, then wait 5 seconds before animating again
  			setPositionByDate(inputDate);
  			stepToCurrentPosition();
  			delay(5000);
  			inputDate = {.d = 0, .m = 0, .y = 0};
  			inputComplete = false;
  			initAnimation();
		}
    	animationStep();	// Just keep animating if there was no input
		delay(2);	// This can be adjusted to make it animate faster or slower
		//delayMicroseconds(1000);	// Minimum is probably like 250 
    #endif
}

// Patterns that all the planets step to while in an animation state, Mercury always steps and MARS steps 1/8 of the time, etc.
byte stepPatterns[4] = { B11111111, B10100100, B10001000, B10000000};
byte currentAnimationStep = 0; // Will always be 0-7

// Makes one animation step, DOES NOT delay at all, must delay outside of this function (calling this in a loop will cause the steppers to miss steps)
void animationStep(){

	// If the position of the current animation step has a 1, make the step
	for(int planet = 0; planet < 4; planet++){
		if(CHECK_BIT(stepPatterns[planet], currentAnimationStep)){
			stepPositions[planet] = (stepPositions[planet] + 1) % MAX_STEP;
			digitalWrite(stepPins[planet], HIGH);
		}
	}
	// Minimum amount of time for the step to successfully happen
	delayMicroseconds(STEP_DELAY);

	// Reset all pins to low
	for(int planet = 0; planet < 4; planet++){
		digitalWrite(stepPins[planet], LOW);
	}
	delayMicroseconds(STEP_DELAY);
	currentAnimationStep = (currentAnimationStep + 1) % 8;
}

// Must be called before attempting to animate
void initAnimation(){
	currentAnimationStep = 0;
    for(int planet = 0; planet < 4; planet++){
		digitalWrite(directionPins[planet], RIGHT);
	}
}

//referenceLoactions {0.491778, 0.2945, 0.0664444, 0.7875};	// Solar lon (0-1) of 01/01/2000
int referenceLoactions[4] = {4917, 2945,  664, 7875};	// Solar lon (0-10000) of 01/01/2000
float orbitalSpeeds[4] = {113.636, 44.5038, 27.3823, 14.556};	// How far a planet moves in a day
Date referenceDate = {.d = 1, .m = 1, .y = 2000};

// Sets 'floatPositions' based on an input date
void setPositionByDate(Date d){

	// Difference in days between given date and reference date
	long days = getDifference(referenceDate, d);

	// Calculate the new position of each planet 
	for(int planet = 0; planet < 4; planet++){
		long newPosition = (referenceLoactions[planet] + (long)(days * orbitalSpeeds[planet])) % 10000;	
		floatPositions[planet] = newPosition / 10000.0;
	}
}

// Calculate how to step to floatPositions, then step there
void stepToCurrentPosition() {

	// Calculate step sequence and direction
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

    // Move the motors
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


// Event to capture date input
void serialEvent() {
	// Date should be input as "YYYY MM DD" or for example "2000 1 1". It is important that there is a newline 
	int inputYear = Serial.parseInt();
	int inputMonth = Serial.parseInt();
	int inputDay = Serial.parseInt();

	// Stick it into the date object
	inputDate = {.d = inputDay, .m = inputMonth, .y = inputYear};

	// Eat any remaining input
	while (Serial.available()) {
		if ((char)Serial.read() == '\n') {
			inputComplete = true;
		}
	}
}


// Date math found here: https://www.geeksforgeeks.org/find-number-of-days-between-two-given-dates/ 
// I'm trusting that it works
const int monthDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; 
// This function counts number of leap years before the given date
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
 
// This function returns number of days between two given dates
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
