#include <iostream> //For errors and warnings
#include <fstream> //For writing log files
#include <ctime> //For logging time
#include <string> //For logging files
#include <unistd.h> //For sleep
#include <ugpio/ugpio.h> //For GPIO
#include <sstream> // For int to string conversion

using namespace std;

//Int to string conversion
template <typename T>
  string NumberToString(T Number) {
    ostringstream ss;
    ss << Number;
    return ss.str();
  }

//Constant global variables declaration
const int totalDirections = 4;
string fileName("log.txt");
const int maxLength = 5; //Max time going straight before
//To keep track of the maze using a spin on Tremaux's algorithm
const int maxWidth = 20;
const int maxHeight = 20;
int allPaths[maxWidth][maxHeight];
//Set starting spot in maze array
int startWidth = maxWidth / 2;
//Current spot in the array
int pathSpot[2] = { startWidth, 0 };

//GPIO values
//IR Sensors
//DIRECTION -> input
//TODO: Set sensor pin numbers
int sensorLeft = 11;
int sensorRight = 19;
int sensorFront = 18;

//Motors
//DIRECTION -> output
//Forward
int motorFL = 3;
int motorFR = 1;
//Reverse
int motorRL = 2;
int motorRR = 0;

//Function Declarations
int initialize();
int changeDirection(int currentDirection, int turnDirection);
int turn(int turnDirection);
void warnMsg(int warnNum, string inFunction, string extra);
void errMsg(int errMsg, string inFunction, string extra);
void writeToLog(string toLog, int type, string extra);
void markPath();
int checkNums(int spot1, int spot2);
int checkTremaux(int left, int straight, int right, int current);
int intersection(int currentDirection);
int checkIR(int irDirection);
int moveForward();
int checkEnd();

/*
DIRECTORY
------------

Direction:
0 - NORTH
1 - EAST
2 - SOUTH
3 - WEST

Turning:
0 - Turn around
1 - Left
2 - Right

Bearings:
0 - straight
1 - Left
2 - Right
3 - Turn around

Logging:
0 - Into Function
1 - Out of function
2 - warnMsg
3 - errMsg
4 - other

*/

int main() {
  string inFunction("main");
  writeToLog(inFunction, 0, "Program start");
  //Initialization
  bool done = false;
  //Initialize the state of all motors to off
  int returnInitialize = initialize();
  if(returnInitialize < 0) {
    errMsg(-1, inFunction, " - failed to initialize all motors to the off state.");
    return -1;
  }
  cout << "Initialized" << endl;
  sleep(1);
  int thing;
  //Show turning capabilities
  thing = turn(1);
  sleep(1);
  thing = turn(2);
  sleep(1);
  thing = turn(0);
  sleep(1);
  int j = 0, returnValue;
  do {
    returnValue = moveForward();
    if(returnValue == 0) {
      //Come to a wall
      j = 0;
      thing = turn(1);
    }
    else if(returnValue == 1) {
      //Stop
      done = true;
    }
    else {
      //Some error, try again
      j ++;
    }
  } while(j < maxLength && !done);
  if(j == maxLength) {
    errMsg(-2, inFunction, " - failed to move forward 5 times.");
    return -2;
  }
  writeToLog(inFunction, 1, "Ending program");
  return 0;
}
/*
initialize
----------
  This function initializes the states of all motors to high (or off).
*/
int initialize() {
  string inFunction("initialize");
  writeToLog(inFunction, 0, "");
  //Initialize all motors
  int rq1, rq2, rq3, rq4;
  int rv1, rv2, rv3, rv4;
  int returnValueFL, returnValueFR, returnValueRL, returnValueRR;

  if((rq1 = gpio_is_requested(motorRL)) < 0 || (rq2 = gpio_is_requested(motorFR)) < 0 || (rq3 = gpio_is_requested(motorRR)) < 0 || (rq4 = gpio_is_requested(motorFL)) < 0) {
    //Error
    errMsg(-2, inFunction, " - the GPIO is already requested.");
    return -2;
  }
  if((!rq1 && (rv1 = gpio_request(motorRL, NULL)) < 0) || (!rq2 && (rv2 = gpio_request(motorFR, NULL)) < 0) || (!rq3 && (rv3 = gpio_request(motorRR, NULL)) < 0) || (!rq4 && (rv4 = gpio_request(motorFL, NULL)) < 0)) {
    //Error
    errMsg(-3, inFunction, " - the GPIO could not be requested.");
    return -3;
  }
  if((rv1 = gpio_direction_output(motorRL, 0)) < 0 || (rv2 = gpio_direction_output(motorFR, 0)) < 0 || (rv3 = gpio_direction_output(motorRR, 0)) < 0 || (rv4 = gpio_direction_output(motorFL, 0)) < 0) {
    //Error
    errMsg(-4, inFunction, " - the GPIO direction could not be set to output.");
    return -4;
  }

  //Set all to high (off)
  returnValueFL = gpio_set_value(motorFL, 1);
  returnValueFR = gpio_set_value(motorFR, 1);
  returnValueRL = gpio_set_value(motorRL, 1);
  returnValueRR = gpio_set_value(motorRR, 1);

  if(returnValueFL < 0 || returnValueFR < 0 || returnValueRL < 0 || returnValueRR < 0) {
    //Reading didn't work
    errMsg(-5, inFunction, " - failed to set values of motors to 1.");
    return -5;
  }
  if((!rq1 && gpio_free(motorRL) < 0) || (!rq2 && gpio_free(motorFR) < 0) || (!rq3 && gpio_free(motorRR) < 0) || (!rq4 && gpio_free(motorFL) < 0)) {
    //Error
    errMsg(-6, inFunction, " - failed to free the GPIOs in initialization.");
    return -6;
  }
  return 0;
}
/*
warnMsg function:
  This function cout's the warning message and then calls the writeToLog function
  to write the warning message in the log file
*/
void warnMsg(int warnNum, string inFunction, string extra) {
  writeToLog("warnMsg", 0, "");
  string toOut = "Warning number " + NumberToString(warnNum) + " occurred in function " + inFunction + extra;
  cerr << "Warning number " << warnNum << " occurred in function " << inFunction << extra << endl;
  writeToLog(toOut, 2, extra);
  writeToLog("warnMsg", 1, "");
}
/*
errMsg function:
  Similar to the warnMsg function, except for error messages
*/
void errMsg(int errNum, string inFunction, string extra) {
  writeToLog("errMsg", 0, "");
  string toOut = "Error number " + NumberToString(errNum) + " occurred in function " + inFunction + extra;
  cerr << "Error number " << errNum << " occurred in function " << inFunction << extra << endl;
  writeToLog(toOut, 3, extra);
  writeToLog("errMsg", 1, "");
}
/*
markPath:
  Marks the path spot according to Tremaux's algorithm
*/
void markPath() {
  string inFunction("markPath");
  writeToLog(inFunction, 0, "");
  allPaths[pathSpot[0]][pathSpot[1]] ++;
  writeToLog(inFunction, 1, "");
}
/*
checkNums:
  Checks the number of marks on the given path spot
*/
int checkNums(int spot1, int spot2) {
  string inFunction("checkNums");
  writeToLog(inFunction, 0, "");
  writeToLog(inFunction, 1, "");
  return allPaths[spot1][spot2];
}
/*
checkTremaux:
  Check all available paths for marks and choose a direction to go (based on algorithm)
*/
int checkTremaux(int left, int straight, int right, int current) {
  string inFunction("checkTremaux");
  writeToLog(inFunction, 0, "");
  if(left >= 2 && straight >= 2 && right >= 2 && current >= 2) {
    //ERROR
    errMsg(1, inFunction, " - An unexpected number was received as a parameter.");
    return -1;
  }
  if(!left) {
    //Go left
    writeToLog(inFunction, 1, "");
    return 1;
  }
  else if(!right) {
    //Go right
    writeToLog(inFunction, 1, "");
    return 2;
  }
  else if(!straight) {
    //Go straight
    writeToLog(inFunction, 1, "");
    return 0;
  }
  else if(current >= 2){
    if(right == 1) {
      //Right
      writeToLog(inFunction, 1, "");
      return 2;
    }
    else if(straight == 1) {
      //Straight
      writeToLog(inFunction, 1, "");
      return 0;
    }
    else if(left == 1) {
      //Left
      writeToLog(inFunction, 1, "");
      return 1;
    }
  }
  else {
    //Turn around
    writeToLog(inFunction, 1, "");
    return 3;
  }
  errMsg(-2, inFunction, " - went past all of the if statements for some reason.");
  return -2;
}
/*
moveForward
-----------
  This function keeps moving the car forward until it detects a path appearing
  on either the left or right side. If it goes straight for a certain amount
  of time, it counts as being out of the maze and returns 1.
*/
int moveForward() {
  string inFunction("moveForward");
  writeToLog(inFunction, 0, "");

  int rq1, rq2, rv1, rv2, returnValueL, returnValueR;
  int done = 0, numFound;
  int irLeft, irRight, irFront, temp, counter, j = 0;
  //Check initial IR states
  //Try checking 5 times
  do {
    irLeft = checkIR(1);
    counter ++;
  } while(irLeft < 0 && counter < 5);
  if(counter == 5) {
    errMsg(-1, inFunction, " - attempt to get left IR reading failed 5 times.");
    return -1;
  }
  //Reset counter
  counter = 0;
  do {
    irRight = checkIR(2);
    counter ++;
  } while(irRight < 0 && counter < 5);
  if(counter == 5) {
    errMsg(-1, inFunction, " - attempt to get right IR reading failed 5 times.");
    return -1;
  }
  counter = 0;
  do {
    irFront = checkIR(0);
    counter ++;
  } while(irFront < 0 && counter < 5);
  if(counter == 5) {
    errMsg(-1, inFunction, " - attempt to get front IR reading failed 5 times.");
    return -1;
  }

  if((rq1 = gpio_is_requested(motorFL)) < 0 || (rq2 = gpio_is_requested(motorFR)) < 0) {
    //Error
    errMsg(-2, inFunction, " - the GPIO is already requested.");
    return -2;
  }
  if((!rq1 && (rv1 = gpio_request(motorFL, NULL)) < 0) || (!rq2 && (rv2 = gpio_request(motorFR, NULL)) < 0)) {
    //Error
    errMsg(-3, inFunction, " - the GPIO could not be requested.");
    return -3;
  }
  if((rv1 = gpio_direction_output(motorFL, 0)) < 0 || (rv2 = gpio_direction_output(motorFR, 0)) < 0) {
    //Error
    errMsg(-4, inFunction, " - the GPIO direction could not be set to output.");
    return -4;
  }
  //Start turning
  counter = 0;
  do {
    returnValueL = gpio_set_value(motorFL, 0);
    returnValueR = gpio_set_value(motorFR, 0);
    counter ++;
  } while(counter < 5 && returnValueL < 0 && returnValueR < 0);
  if(counter == 5) {
    //Didn't work
    errMsg(-5, inFunction, " - failed to set the motors to LOW state.");
    return -5;
  }
  //Continue moving forward until a new pathway is detected
  cout << "Starting..." << endl;
  cout << "irFront: " << irFront << " temp : " << temp << endl;
  do {
    counter = 0;
    do {
      temp = checkIR(0);
      counter ++;
    } while(irFront < 0 && counter < 5);
    if(counter == 5) {
      errMsg(-1, inFunction, " - attempt to get front IR reading failed 5 times.");
      return -1;
    }
    if(temp != 0) {
      //Some change on the front side
      if(irFront) {
        //A wall appeared. Currently leaving an intersection
        irFront = 0;
      }
      else {
        //A pathway appeared. Stop and determine where to turn
      	done ++;
      	continue;
      }
    }
    else {
      //No wall actually appeared before (if it detected one before) so reset variable
      done = 0;
    }
    j ++;
  } while(done < 2 && j < 10000);
  if(j == 10000) {
    //End of maze
    return 1;
  }
  //Stop turning
  do {
    returnValueL = gpio_set_value(motorFL, 1);
    returnValueR = gpio_set_value(motorFR, 1);
  } while(counter < 5 && returnValueL < 0 && returnValueR < 0);
  if(counter == 5) {
    //Didn't work
    errMsg(-6, inFunction, " - failed to set the motors to HIGH state.");
    return -6;
  }

  if((!rq1 && gpio_free(motorRL) < 0) || (!rq2 && gpio_free(motorFR) < 0)) {
    //Error
    errMsg(-7, inFunction, " - failed to free GPIOs.");
    return -7;
  }
  return 0;
}
/*
intersection:
  Calls other functions to check IR sensors for available paths then decides
  where to go next, then goes in that direction.
*/
int intersection(int currentDirection) {
  string inFunction("intersection");
  writeToLog(inFunction, 0, "");
  //Initial error check
  if(currentDirection < 0 || currentDirection > 3) {
    errMsg(-1, inFunction, " - an unexpected direction was received as a parameter.");
    return -1;
  }

  int straight, left, right, turnDirection, returnValue;
  int counter = 0;
  bool turnAround = false;

  //Mark the corner of the path just came out of
  markPath();
  //Try checking IR sensors 5 times each
  do {
    returnValue = checkIR(1);
    counter ++;
  } while(counter < 5 && returnValue < 0);
  if(counter == 5) {
    errMsg(-2, inFunction, " - Failed to get a reading from the left IR sensor 5 times.");
    return -2;
  }
  //If true, there exists a path and can possibly go down it
  if(returnValue) {
    left = checkNums(pathSpot[0] - 1, pathSpot[1] + 1);
  }
  //If false, there is a wall.
  else {
    left = 3;
  }
  //Reset counter
  counter = 0;
  do {
    returnValue = checkIR(0);
    counter ++;
  } while(counter < 5 && returnValue < 0);
  if(counter == 5) {
    errMsg(-2, inFunction, " - Failed to get a reading from the front IR sensor 5 times.");
    return -2;
  }
  if(returnValue) {
    straight = checkNums(pathSpot[0], pathSpot[1] + 2);
  }
  else {
    straight = 3;
  }
  counter = 0;
  do {
    returnValue = checkIR(2);
    counter ++;
  } while(counter < 5 && returnValue < 0);
  if(counter == 5) {
    errMsg(-2, inFunction, " - Failed to get a reading from the right IR sensor 5 times.");
    return -2;
  }
  if(returnValue) {
    right = checkNums(pathSpot[0] + 1, pathSpot[1] + 1);
  }
  else {
    right = 3;
  }
  //Get current location
  int current = checkNums(pathSpot[0], pathSpot[1]);
  //Using the algorithm decide which direction to turn based on what is available
  turnDirection = checkTremaux(left, straight, right, current);
  switch(currentDirection) {
    case 0: //North
      if(turnDirection == 1) {
        pathSpot[0] -= 1;
        pathSpot[1] += 1;
      }
      else if(turnDirection == 2) {
        pathSpot[0] += 1;
        pathSpot[1] += 1;
      }
      else if(!turnDirection) {
        pathSpot[1] += 2;
      }
      else {
        turnAround = true;
      }
      break;
    case 1: //East
      if(turnDirection == 1) {
        pathSpot[1] += 2;
      }
      else if(turnDirection == 2) {
        pathSpot[1] -= 2;
      }
      else if(!turnDirection) {
        pathSpot[0] += 1;
        pathSpot[1] += 1;
      }
      else {
        turnAround = true;
      }
      break;
    case 2: //South
      if(turnDirection == 2) {
        pathSpot[0] -= 1;
        pathSpot[1] += 1;
      }
      else if(turnDirection == 1) {
        pathSpot[0] += 1;
        pathSpot[1] += 1;
      }
      else if(!turnDirection) {
        pathSpot[1] -= 2;
      }
      else {
        turnAround = true;
      }
      break;
    case 3: //West
      if(turnDirection == 2) {
        pathSpot[1] += 2;
      }
      else if(turnDirection == 1) {
        pathSpot[1] -= 2;
      }
      else if(!turnDirection) {
        pathSpot[0] -= 1;
        pathSpot[1] -= 1;
      }
      else {
        turnAround = true;
      }
      break;
  }
  if(turnAround) {
    currentDirection = changeDirection(currentDirection, 0);
    markPath();
    markPath();
    //Dead end so mark it twice so that the car doesn't come back down this path.
    writeToLog(inFunction, 1, "");
    return currentDirection;
  }
  markPath(); //Increment spot in allPaths array
  //Change direction
  if(turnDirection) {
    currentDirection = changeDirection(currentDirection, turnDirection);
  }
  //Go to next intersection
  if(currentDirection == 1) {
    pathSpot[0] += 1;
  }
  else if(currentDirection == 3) {
    pathSpot[0] -= 1;
  }
  else if(currentDirection == 0) {
    pathSpot[1] += 1;
  }
  else {
    pathSpot[1] -= 1;
  }
  writeToLog(inFunction, 1, "");
  return currentDirection;
}
/*
changeDirection:
  Changes the orientation of the car (to keep track of it) whenever the car turns
*/
int changeDirection(int currentDirection, int turnDirection) {
  string inFunction("changeDirection");
  writeToLog(inFunction, 0, "");
  //Initial error checking
  if(currentDirection < 0 || currentDirection > 3) {
    errMsg(1, inFunction, " - an unexpected direction was received.");
    return -1;
  }
  int returnValue, counter = 0;
  //Try to turn the car 5 times
  do {
    returnValue = turn(turnDirection);
    //If there is an error, output it and try again
    if(returnValue < 0) {
      warnMsg(returnValue, inFunction, " - failed to turn car.");
    }
    //Keep track of the current path spot and orientation after turning
    switch(turnDirection) {
      case 0:
        //Turn around
        if(returnValue < 0) {
          return returnValue;
        }
        writeToLog(inFunction, 1, "");
        return (currentDirection + 2) % totalDirections;
        break;
      case 1:
        //Turn left
        if(returnValue < 0) {
          return returnValue;
        }
        if(!currentDirection) {
          return 4;
        }
        writeToLog(inFunction, 1, "");
        return (currentDirection - 1) % totalDirections;
        break;
      case 2:
        //Turn right
        if(returnValue < 0) {
          return returnValue;
        }
        writeToLog(inFunction, 1, "");
        return (currentDirection + 1) % totalDirections;
        break;
    }
    counter ++;
    if(counter < 5 && returnValue < 0) {
      cout << "Trying again in one second..." << endl;
    }
    //Wait one second to try again;
    sleep(1);
  } while(returnValue < 0 && counter < 5);
  if(counter == 5) {
    //Tried to turn 5 times, didn't work
    errMsg(-10, inFunction, " - failed to turn the car 5 times.");
    return -10;
  }
  //Somehow exited loop without returning a value
  errMsg(-11, inFunction, " - made it past do..while loop somehow.");
  return -11;
}
/*
checkIR:
  Check the values of each of the IR sensors to see if there is a path available
*/
int checkIR(int irDirection) {
  string inFunction("checkIR");
  writeToLog(inFunction, 0, "");

  int returnValue, rv, rq;
  int sensor, counter = 0;
  //Figure out which sensor is requested
  if(irDirection == 1) {
    //Left
    sensor = sensorLeft;
  }
  else if(irDirection == 2) {
    //Right
    sensor = sensorRight;
  }
  else if(irDirection == 0) {
    //Straight
    sensor = sensorFront;
  }
  //Receive signal from IR sensors
  if((rq = gpio_is_requested(sensor)) < 0) {
    //Error
    errMsg(-2, inFunction, " - the GPIO is already requested.");
    return -2;
  }
  if(!rq && (rv = gpio_request(sensor, NULL)) < 0) {
    //Error
    errMsg(-3, inFunction, " - the GPIO could not be requested.");
    return -3;
  }
  if((rv = gpio_direction_input(sensor)) < 0) {
    //Error
    errMsg(-4, inFunction, " - the GPIO direction could not be set to input.");
    return -4;
  }
  //Try getting value 5 times
  do {
    returnValue = gpio_get_value(sensor);
    counter ++;
  } while(returnValue < 0 && counter < 5);
  if(counter == 5) {
    //Failed 5 times
    errMsg(-5, inFunction, " - failed to get IR sensor value 5 times.");
    return -5;
  }

  writeToLog(inFunction, 1, "");
  //If it senses something, return false
  if(returnValue) {
    return false;
  }
  //If there is no wall, return true
  return true;
}
/*
turn:
  Send signals to the motors to turn the car
*/
int turn(int turnDirection) {
  string inFunction("turn");
  writeToLog(inFunction, 0, "");
  //Initial error checking
  if(turnDirection < 0 || turnDirection > 2) {
    errMsg(1, inFunction, " - unexpected turn direction received as parameter.");
    return -1;
  }
  int returnValueL, returnValueR, rq1, rq2, rv1, rv2, counter = 0, returnEnd;
  //Milliseconds to turn designated degrees
  int val90Deg = 2;
  int val180Deg = 2 * val90Deg;

  if(turnDirection == 0) {
    //Turn around
    if((rq1 = gpio_is_requested(motorRL)) < 0 || (rq2 = gpio_is_requested(motorFR)) < 0) {
      //Error
      errMsg(-2, inFunction, " - the GPIO is already requested.");
      return -2;
    }
    if((!rq1 && (rv1 = gpio_request(motorRL, NULL)) < 0) || (!rq2 && (rv2 = gpio_request(motorFR, NULL)) < 0)) {
      //Error
      errMsg(-3, inFunction, " - the GPIO could not be requested.");
      return -3;
    }
    if((rv1 = gpio_direction_output(motorRL, 0)) < 0 || (rv2 = gpio_direction_output(motorFR, 0)) < 0) {
      //Error
      errMsg(-4, inFunction, " - the GPIO direction could not be set to output.");
      return -4;
    }
    //Start turning
    returnValueL = gpio_set_value(motorRL, 0);
    returnValueR = gpio_set_value(motorFR, 0);

    if(returnValueL < 0 || returnValueR < 0) {
      //Reading didn't work
      errMsg(-5, inFunction, " - failed to set motor states to LOW.");
      return -5;
    }
    //Keep turning for the right amount of milliseconds
    sleep(val180Deg);
    //Stop turning
    returnValueL = gpio_set_value(motorRL, 1);
    returnValueR = gpio_set_value(motorFR, 1);
    if(returnValueL < 0 || returnValueR < 0) {
      //Reading didn't work
      errMsg(-5, inFunction, " - failed to set motor states to HIGH.");
      return -5;
    }
    if((!rq1 && gpio_free(motorRL) < 0) || (!rq2 && gpio_free(motorFR) < 0)) {
      //Error
      errMsg(-6, inFunction, " - failed to free GPIOs");
      return -6;
    }
  }
  else if(turnDirection == 1) {
    //Turn left
    if((rq1 = gpio_is_requested(motorRL)) < 0 || (rq2 = gpio_is_requested(motorFR)) < 0) {
      //Error
      errMsg(-2, inFunction, " - the GPIO is already requested.");
      return -2;
    }
    if((!rq1 && (rv1 = gpio_request(motorRL, NULL)) < 0) || (!rq2 && (rv2 = gpio_request(motorFR, NULL)) < 0)) {
      //Error
      errMsg(-3, inFunction, " - the GPIO could not be requested.");
      return -3;
    }
    if((rv1 = gpio_direction_output(motorRL, 0)) < 0 || (rv2 = gpio_direction_output(motorFR, 0)) < 0) {
      //Error
      errMsg(-4, inFunction, " - the GPIO direction could not be set to output.");
      return -4;
    }
    returnValueL = gpio_set_value(motorRL, 0);
    returnValueR = gpio_set_value(motorFR, 0);
    if(returnValueL < 0 || returnValueR < 0) {
      //Reading didn't work
      errMsg(-5, inFunction, " - failed to set motor states to LOW.");
      return -5;
    }
    counter = 0;
    cout << "Here" << endl;
    sleep(val90Deg);
    //Stop turning
    returnValueL = gpio_set_value(motorRL, 1);
    returnValueR = gpio_set_value(motorFR, 1);
    if(returnValueL < 0 || returnValueR < 0) {
      //Reading didn't work
      errMsg(-5, inFunction, " - failed to set motor states to HIGH.");
      return -5;
    }
    if((!rq1 && gpio_free(motorRL) < 0) || (!rq2 && gpio_free(motorFR) < 0)) {
      //Error
      errMsg(-6, inFunction, " - failed to free GPIOs");
      return -6;
    }
  }
  else if(turnDirection == 2){
    //Turn right
    if((rq1 = gpio_is_requested(motorFL)) < 0 || (rq2 = gpio_is_requested(motorRR)) < 0) {
      //Error
      errMsg(-2, inFunction, " - the GPIO is already requested.");
      return -2;
    }
    if((!rq1 && (rv1 = gpio_request(motorFL, NULL)) < 0) || (!rq2 && (rv2 = gpio_request(motorRR, NULL)) < 0)) {
      //Error
      errMsg(-3, inFunction, " - the GPIO could not be requested.");
      return -3;
    }
    if((rv1 = gpio_direction_output(motorFL, 0)) < 0 || (rv2 = gpio_direction_output(motorRR, 0)) < 0) {
      //Error
      errMsg(-4, inFunction, " - the GPIO direction could not be set to output.");
      return -4;
    }
    returnValueL = gpio_set_value(motorFL, 0);
    returnValueR = gpio_set_value(motorRR, 0);
    if(returnValueL < 0 || returnValueR < 0) {
      //Reading didn't work
      errMsg(-5, inFunction, " - failed to set motor states to LOW.");
      return -5;
      return -5;
    }
    sleep(val90Deg);
    //Stop turning
    returnValueL = gpio_set_value(motorFL, 1);
    returnValueR = gpio_set_value(motorRR, 1);
    if(returnValueL < 0 || returnValueR < 0) {
      //Reading didn't work
      errMsg(-5, inFunction, " - failed to set motor states to HIGH.");
      return -5;
    }
    if((!rq1 && gpio_free(motorFL) < 0) || (!rq2 && gpio_free(motorRR) < 0)) {
      //Error
      errMsg(-6, inFunction, " - failed to free GPIOs");
      return -6;
    }
  }
  writeToLog(inFunction, 1, "");
  return 0;
}
/*
writeToLog:
  Write the string received as parameter to the log file
*/
void writeToLog(string toLog, int type, string extra) {
  ofstream out;
  //Create the file if it doesn't exist. If it does, append
  out.open(fileName.c_str(), ios::app);
  if(!out.is_open()) {
    errMsg(-1, "writeToLog", ""); //Unexpected file name
  }
  time_t now = time(0); //Current time
  char *outTime = ctime(&now);
  //Output outTime to log file
  out << outTime;
  switch(type) {
    case 0:
      out << "Entering function " << toLog << endl;
      out << extra << endl;
      break;
    case 1:
      out << "Leaving function " << toLog << endl;
      out << extra << endl;
      break;
    case 2:
    case 3:
    case 4:
      out << toLog << endl;
      out << extra << endl;
      break;
  }
  out.close();
}
