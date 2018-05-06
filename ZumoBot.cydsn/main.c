/**
* @mainpage ZumoBot Project
* @brief    You can make your own ZumoBot with various sensors.
* @details  <br><br>
    <p>
    <B>General</B><br>
    You will use Pololu Zumo Shields for your robot project with CY8CKIT-059(PSoC 5LP) from Cypress semiconductor.This 
    library has basic methods of various sensors and communications so that you can make what you want with them. <br> 
    <br><br>
    </p>
    
    <p>
    <B>Sensors</B><br>
    &nbsp;Included: <br>
        &nbsp;&nbsp;&nbsp;&nbsp;LSM303D: Accelerometer & Magnetometer<br>
        &nbsp;&nbsp;&nbsp;&nbsp;L3GD20H: Gyroscope<br>
        &nbsp;&nbsp;&nbsp;&nbsp;Reflectance sensor<br>
        &nbsp;&nbsp;&nbsp;&nbsp;Motors
    &nbsp;Wii nunchuck<br>
    &nbsp;TSOP-2236: IR Receiver<br>
    &nbsp;HC-SR04: Ultrasonic sensor<br>
    &nbsp;APDS-9301: Ambient light sensor<br>
    &nbsp;IR LED <br><br><br>
    </p>
    
    <p>
    <B>Communication</B><br>
    I2C, UART, Serial<br>
    </p>
*/

#include <project.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "Systick.h"
#include "Motor.h"
#include "Ultra.h"
#include "Nunchuk.h"
#include "Reflectance.h"
#include "I2C_made.h"
#include "Gyro.h"
#include "Accel_magnet.h"
#include "IR.h"
#include "Ambient.h"
#include "Beep.h"
#include <time.h>
#include <sys/time.h>
int rread(void);

/**
 * @file    main.c
 * @brief   
 * @details  ** Enable global interrupt since Zumo library uses interrupts. **<br>&nbsp;&nbsp;&nbsp;CyGlobalIntEnable;<br>
*/


#if 0 //SUMO
int main()
{
    struct sensors_ ref; //struct to save all the uint16_t values of all the 6 reflectance sensor
    struct sensors_ dig; //all digital (0 or 1) values of the 6 reflectance sensors
    
    CyGlobalIntEnable; 
    UART_1_Start(); //serial port communication
    Systick_Start(); //start system timer
    IR_Start(); //start infrared sensor
    ADC_Battery_Start(); //start battery checking
    Ultra_Start(); // Ultra Sonic Start function

    //battery variables
    int time = 0; //time in seconds
    int timesCheckedBattery = 1; //how many times the battery level has been checked
    int ledOn = 0; //Is the battery led on? (0 = no, 1 = yes)
    int16 adcresult = 0; //battery level from 0 to 4095
    float volts = 5.0; //battery level in volts
    
    //motor variables
    int driveDelay = 2; //how long are the motors driven for with 1 setting before new values are calculated
    int maxSpeed = 255; //maximum speed for the motors
    
    //sumo variables
    int distance = 0; //ultra sonic sensor distance
    int random = 0; //a random number
    int randomDelay = 10; //random delay
    int distanceTreshold = 15; //the range of ultra sonic sensor
    int direction = 0; //direction left = 0, right = 1
    int turnSpeed = 255; //turning speed 0-255
    
    reflectance_start(); //reflectance sensor start
    CyDelay(20); //wait for 20 ms
    reflectance_set_threshold(9000, 10000, 10000, 10000, 10000, 9000); // set edge sensor thresholds to 9000 and others to 10000
    
    printf("\nBEEP BOOP\n"); //write something to show that boot happened
    BatteryLed_Write(0); // Switch led off 
    
    motor_start(); //start the motors
    PWM_WriteCompare1(0); //set left motor speed to 0
    PWM_WriteCompare2(0); //set right motor speed to 0
    
    for(;;) //first loop to drive until the first line is seen and then wait for infrared signal
    {
        reflectance_read(&ref); //update reflectance sensor values
        reflectance_digital(&dig); //update refelectance sensor values depending on the threshold values (digital)
        
        //when button is pressed and sensors see black, drive forward with speed 100
        if (SW1_Read() == 0 && (dig.l3 == 1 || dig.l2 == 1 || dig.l1 == 1 || dig.r1 == 1 || dig.r2 == 1 || dig.r3 == 1))        
        {
            MotorDirLeft_Write(0); 
            MotorDirRight_Write(0); 
            PWM_WriteCompare1(100); 
            PWM_WriteCompare2(100); 
        }
        //stop when all sensors see black and wait for infrared singal
        if (dig.l1 == 1 && dig.l2 == 1 && dig.l3 == 1 && dig.r1 == 1 && dig.r2 == 1 && dig.r3 == 1)
        {       
            PWM_WriteCompare1(0); 
            PWM_WriteCompare2(0);
            IR_wait(); 
            break;
        }
    }
    //when infrared signal is read, drive forward for 500ms
    MotorDirLeft_Write(0); 
    MotorDirRight_Write(0);
    PWM_WriteCompare1(maxSpeed);
    PWM_WriteCompare2(maxSpeed);
    CyDelay(500);
            
    for(;;) //second for-loop for designating the logic when within the sumo ring
    {
        reflectance_read(&ref); //update reflectance sensor values
        reflectance_digital(&dig); //update refelectance sensor values depending on the threshold values (digital)
        distance = Ultra_GetDistance(); //save ultra sensor distance value to distance variable
        srand(GetTicks()); //seed the random value generator depending on the current time
             
        // if only the leftmost sensor sees black
        if (dig.l3 == 1 && dig.l2 == 0 && dig.l1 == 0 && dig.r1 == 0 && dig.r2 == 0 && dig.r3 == 0)
        {   
            //drive backwards while turning away from the black line for random time between 300-600ms
            MotorDirLeft_Write(1); 
            MotorDirRight_Write(1);
            PWM_WriteCompare1(maxSpeed); 
            PWM_WriteCompare2(150);
            random = rand() % 601; //random number between 0 and 600
            
            if (random < 300) //minimum number for random
            {
                random = 300;
            }
            CyDelay(random);
            
            //random number between 0 and 1
            random = rand() % 2;
            
            //excecute if random number is 0
            if (random == 0)
            {
                //random number between 0-500;
                random = rand() % 501;
                randomDelay = random/10;
                
                //Spin in place counterclockwise for the time determined by randomDelay. Then check the distance from the ultrasonic sensor.
                //If the distance read from ultrasonic sensor is less than the treshold determined earlier, drive forward with full speed
                //in an effort to push the opponent out of the ring. If no enemy is seen, return to spinning counterclockwise. Spin ends when
                //random is lower than 10 or enemy is seen.
                while (random > 10)
                {
                    MotorDirLeft_Write(1); 
                    MotorDirRight_Write(0);
                    PWM_WriteCompare1(maxSpeed); 
                    PWM_WriteCompare2(maxSpeed);
                    CyDelay(randomDelay);
                    random -= randomDelay;
                    distance = Ultra_GetDistance();
                    
                    if (distance < distanceTreshold)
                    {
                        MotorDirLeft_Write(0); 
                        MotorDirRight_Write(0);
                        PWM_WriteCompare1(maxSpeed); 
                        PWM_WriteCompare2(maxSpeed);
                        CyDelay(50);
                        random = 0;
                    }
                }
            }
            //excute if random number is 1
            else
            {
                //random number between 0-500;
                random = rand() % 501;
                randomDelay = random/10;
                
                //Spin in place clockwise for the time determined by randomDelay. Then check the distance from the ultrasonic sensor.
                //If the distance read from ultrasonic sensor is less than the treshold determined earlier, drive forward with full speed
                //in an effort to push the opponent out of the ring. If no enemy is seen, return to spinning clockwise. Spin ends when
                //random is lower than 10 or enemy is seen..
                while (random > 10)
                {
                    MotorDirLeft_Write(0); 
                    MotorDirRight_Write(1);
                    PWM_WriteCompare1(maxSpeed); 
                    PWM_WriteCompare2(maxSpeed);
                    CyDelay(randomDelay);
                    random -= randomDelay;
                    distance = Ultra_GetDistance();                    
                    
                    if (distance < distanceTreshold)
                    {
                        MotorDirLeft_Write(0); 
                        MotorDirRight_Write(0);
                        PWM_WriteCompare1(maxSpeed); 
                        PWM_WriteCompare2(maxSpeed);
                        CyDelay(50);
                        random = 0;
                    }
                }
            }  
        }
        // if only the rightmost sensor sees black
        else if(dig.l1 == 0 && dig.l2 == 0 && dig.l3 == 0 && dig.r1 == 0 && dig.r2 == 0 && dig.r3 == 1) 
        {   
            //drive backwards while turning away from the black line for random time between 300-600ms
            MotorDirLeft_Write(1); 
            MotorDirRight_Write(1);
            PWM_WriteCompare1(150);           
            PWM_WriteCompare2(maxSpeed);
            random = rand() % 601;
           
            if (random < 300)
            {
                random = 300;
            }
            CyDelay(random);
            
            //random number between 0 and 1
            random = rand() % 2;
            
            //excecute if random number is 0
            if (random == 0)
            {
                //random number between 0-500;
                random = rand() % 501;
                randomDelay = random/10;
                
                //Spin in place counterclockwise for the time determined by randomDelay. Then check the distance from the ultrasonic sensor.
                //If the distance read from ultrasonic sensor is less than the treshold determined earlier, drive forward with full speed
                //in an effort to push the opponent out of the ring. If no enemy is seen, return to spinning counterclockwise. Spin ends when
                //random is lower than 10 or enemy is seen.
                while (random > 10)
                {
                    MotorDirLeft_Write(1); 
                    MotorDirRight_Write(0);
                    PWM_WriteCompare1(maxSpeed); 
                    PWM_WriteCompare2(maxSpeed);
                    CyDelay(randomDelay);
                    random -= randomDelay;
                    distance = Ultra_GetDistance();
                    
                    if (distance < distanceTreshold)
                    {
                        MotorDirLeft_Write(0); 
                        MotorDirRight_Write(0);
                        PWM_WriteCompare1(maxSpeed); 
                        PWM_WriteCompare2(maxSpeed);
                        CyDelay(50);
                        random = 0;
                    }
                }
            }
            
            //excute if random number is 1
            else
            {
                //random number between 0-500;
                random = rand() % 501;
                randomDelay = random/10;
                
                //Spin in place clockwise for the time determined by randomDelay. Then check the distance from the ultrasonic sensor.
                //If the distance read from ultrasonic sensor is less than the treshold determined earlier, drive forward with full speed
                //in an effort to push the opponent out of the ring. If no enemy is seen, return to spinning clockwise. Spin ends when
                //random is lower than 10 or enemy is seen.
                while (random > 10)
                {
                    MotorDirLeft_Write(0); 
                    MotorDirRight_Write(1);
                    PWM_WriteCompare1(maxSpeed); 
                    PWM_WriteCompare2(maxSpeed);
                    CyDelay(randomDelay);
                    random -= randomDelay;
                    distance = Ultra_GetDistance();                    
                    
                    if (distance < distanceTreshold)
                    {
                        MotorDirLeft_Write(0); 
                        MotorDirRight_Write(0);
                        PWM_WriteCompare1(maxSpeed); 
                        PWM_WriteCompare2(maxSpeed);
                        CyDelay(50);
                        random = 0;
                    }
                }
            }                  
        }
        
        //if any of the sensors see black
        else if (dig.l2 == 1 || dig.l1 == 1 || dig.r1 == 1 || dig.r2 == 1) 
        {
            //drive backwards for random time between 200-500ms
            MotorDirLeft_Write(1); 
            MotorDirRight_Write(1);
            PWM_WriteCompare1(maxSpeed); 
            PWM_WriteCompare2(maxSpeed);
            random = rand() % 501;
            
            if (random < 200)
            {
                random = 200;
            }
            CyDelay(random);
            
            //random number between 0 and 1
            random = rand() % 2;
            
            //excecute if random number is 0
            if (random == 0)
            {
                //random number between 0-500;
                random = rand() % 501;
                randomDelay = random/10;
                
                //Spin in place counterclockwise for the time determined by randomDelay. Then check the distance from the ultrasonic sensor.
                //If the distance read from ultrasonic sensor is less than the treshold determined earlier, drive forward with full speed
                //in an effort to push the opponent out of the ring. If no enemy is seen, return to spinning counterclockwise. Spin ends when
                //random is lower than 10 or enemy is seen.
                while (random > 10)
                {
                    MotorDirLeft_Write(1); 
                    MotorDirRight_Write(0);
                    PWM_WriteCompare1(maxSpeed); 
                    PWM_WriteCompare2(maxSpeed);
                    CyDelay(randomDelay);
                    random -= randomDelay;
                    distance = Ultra_GetDistance();
                    
                    if (distance < distanceTreshold)
                    {
                        MotorDirLeft_Write(0); 
                        MotorDirRight_Write(0);
                        PWM_WriteCompare1(maxSpeed); 
                        PWM_WriteCompare2(maxSpeed);
                        CyDelay(50);
                        random = 0;
                    }
                }
            }
            
            //excute if random number is 1
            else
            {
                //random number between 0-500;
                random = rand() % 501;
                randomDelay = random/10;
                
                //Spin in place clockwise for the time determined by randomDelay. Then check the distance from the ultrasonic sensor.
                //If the distance read from ultrasonic sensor is less than the treshold determined earlier, drive forward with full speed
                //in an effort to push the opponent out of the ring. If no enemy is seen, return to spinning clockwise. Spin ends when
                //random is lower than 10 or enemy is seen.
                while (random > 10)
                {
                    MotorDirLeft_Write(0); 
                    MotorDirRight_Write(1);
                    PWM_WriteCompare1(maxSpeed); 
                    PWM_WriteCompare2(maxSpeed);
                    CyDelay(randomDelay);
                    random -= randomDelay;
                    distance = Ultra_GetDistance();                    
                    
                    if (distance < distanceTreshold)
                    {
                        MotorDirLeft_Write(0); 
                        MotorDirRight_Write(0);
                        PWM_WriteCompare1(maxSpeed); 
                        PWM_WriteCompare2(maxSpeed);
                        CyDelay(50);
                        random = 0;
                    }
                }
            }           
        }
        
        //if enemy is detected with ultrasonic sensor, drive forward with full speed
        else if (distance < distanceTreshold)
        {       
            MotorDirLeft_Write(0); 
            MotorDirRight_Write(0);
            PWM_WriteCompare1(255); 
            PWM_WriteCompare2(255);
            CyDelay(driveDelay);
        }
        
        //if no black line or enemies are seen, drive forward zig-zagging.
        else
        {   
            //reduce speed from the left engine
            if (direction == 0)
            {
                turnSpeed -= 4;
                MotorDirLeft_Write(0); 
                MotorDirRight_Write(0);
                PWM_WriteCompare1(turnSpeed);
                PWM_WriteCompare2(maxSpeed);
                CyDelay(10);
                
                //when the left engine is slow enough, turn direction and reset turn speed
                if (turnSpeed < 10)
                {
                    direction = 1;
                    turnSpeed = 255;
                }
            }
            
            //reduce speed from the right engine
            else if (direction == 1)
            {
                turnSpeed -= 4;
                MotorDirLeft_Write(0); 
                MotorDirRight_Write(0);
                PWM_WriteCompare1(maxSpeed);
                PWM_WriteCompare2(turnSpeed);
                CyDelay(10);
                
                //when the right engine is slow enough, turn direction and reset turn speed
                if (turnSpeed < 10)
                {
                    direction = 0;
                    turnSpeed = 255;
                }
            }
        }
            
        //Battery + LED
        time = GetTicks()/1000; //seconds
        
        if (time > (60*timesCheckedBattery)) //go here every 60 seconds
        {
            ADC_Battery_StartConvert();
            
            if(ADC_Battery_IsEndConversion(ADC_Battery_WAIT_FOR_RESULT)) // wait for get ADC converted value
            {   
                adcresult = ADC_Battery_GetResult16(); // get the ADC value (0 - 4095)
                volts = (float) adcresult/4095*5*1.5; //scale the ADC result value from 0-4095 to 0-5V and multiply it by 1.5 to take into account the voltage reduction in the microcontroller
            }
            timesCheckedBattery++; //variable that tells how many times the battery has been checked. Used for checking the battery every 5 seconds.
        }
        if (volts < 4) //if battery is too low flash the led
        {
            if (ledOn == 0) //if led is off, turn the led on
            {
                BatteryLed_Write(1);
                ledOn = 1;
            }
            else //if led is on turn the led off
            {
                BatteryLed_Write(0);
                ledOn = 0;
            }
            CyDelay(10);
        }
        if (volts >= 4 && ledOn == 1) //if battery is back to over 4 and led was left on, turn it off
        {
            BatteryLed_Write(0);
            ledOn = 0;
        }
    }
    motor_stop();  //stop the motor
    
}
         
#endif

#if 1 //PD LINE
int main()
{
    struct sensors_ ref; //struct to save all the uint16_t values of all the 6 reflectance sensor
    
    CyGlobalIntEnable; 
    UART_1_Start(); //serial port communication
    Systick_Start(); //start system timer
    IR_Start(); //start infrared sensor
    ADC_Battery_Start(); //start battery checking

    //battery variables
    int time = 0; //time in seconds
    int timesCheckedBattery = 1; //how many times the battery level has been checked
    int ledOn = 0; //Is the battery led on? (0 = no, 1 = yes)
    int16 adcresult = 0; //battery level from 0 to 4095
    float volts = 5.0; //battery level in volts
    
    int counter = 0; //counter for printing reflectance sensor variables
    
    //motor variables
    int driveDelay = 2; //how long are the motors driven for with 1 setting before new values are calculated
    int maxSpeed = 255; //maximum speed for the motors
    int lastSeenDirection = 0; //last seen direction of the line (1 = left, 0 = right)
    int timesSeenBlackLine = 0; //how many times has a black line been seen (save to stop at the 3rd line)
    int lastSeenBlackLine = 0; //if this is 1 then we saw a black line during the last check so we do not count another black line because we are still on top of the same one we just added
    int blackLineTreshold = 10000; //black line counter treshold for the sensors
    
    //PD controller variables
    int Kp = 20, Kd = 80; //Kp and Kd variables
    int errorLeft = 0, errorRight = 0; //Proportional error for the left most center reflectance sensor and the right most reflectance sensor
    int lastErrorLeft = 0, lastErrorRight = 0; //The previous error left and right
    double motorSpeed = 0; //PD controller chosen motor speed (double)
    int motorSpeedInt = 0; //PD controller chosen motor speed (int)
    int refLeft = 16000, refRight = 16000; //when the robot is in the middle of the line both sensors see a bit of white because they aren't in the middle of the line
    int maxRef = 16000; //max reflectance sensor value which is considered as a "not full black"
    int pwmScaler = 260000; //the number that is used to scale the error value to 0-255 pwm motor speed.
    
    reflectance_start(); //reflectance sensor start
    CyDelay(20); //wait for 20 ms
    reflectance_set_threshold(9000, 10000, 10000, 10000, 10000, 9000); // set edge sensor thresholds to 9000 and others to 10000
    
    printf("\nBEEP BOOP\n"); //write something to show that boot happened
    BatteryLed_Write(0); // Switch led off 
    
    motor_start(); //start the motor
    PWM_WriteCompare1(0); //set left motor speed to 0
    PWM_WriteCompare2(0); //set right motor speed to 0
    
    for(;;) //first loop to drive until the first line is seen and then wait for infrared signal
    {
        reflectance_read(&ref); //update reflectance sensor values
        motor_forward(100,driveDelay); //Drive forwards for driveDelay ms with a speed of 100 (0-255)
        if(ref.l3 > blackLineTreshold && ref.r3 > blackLineTreshold) //if left and right most reflectance sensors see black then break (we see a line)
        {
            PWM_WriteCompare1(0); //set left motor speed to 0
            PWM_WriteCompare2(0); //set right motor speed to 0
            lastSeenBlackLine = 1; //we saw a black line the last time
            timesSeenBlackLine = 1; //we have seen the black line 1 time
            break; //go away from the infinite for loop
        }
    }
    
    IR_flush(); // clear IR receive buffer
    IR_wait(); // wait for IR command
    
    for(;;) //The main line following loop.
    {
        //Line reading with motor control
        // read raw sensor values
        reflectance_read(&ref);
        /*
        if (counter == 12) //do not print every time the main loop is gone through to make reading the values easier
        {
            //printf("Vasemmat: L3: %5d L2: %5d L1: %5d oik.: R1: %5d R2: %5d R3: %5d \n", ref.l3, ref.l2, ref.l1, ref.r1, ref.r2, ref.r3);
            printf("%5d %5d %5d %5d %5d %5d\r\n", ref.l3, ref.l2, ref.l1, ref.r1, ref.r2, ref.r3);
            counter = 0;
        }
        */
        
        counter++; //print reading counter +1
        
        //PID controller (PD controller)
		//We only use the 2 reflectance sensors in the middle
        if (ref.l1 > maxRef) //if reflectance sensor sees a larger number than 16000 then make it 16000
        {
           refLeft = maxRef;
        }
        else
        {
            refLeft = ref.l1;
        }
        if (ref.r1 > maxRef) //if reflectance sensor sees a larger number than 16000 then make it 16000
        {
            refRight = maxRef;
        }
        else
        {
            refRight = ref.r1;
        }
        errorLeft = maxRef - refLeft; //left sensor error is 16000 - "the value the left sensor sees"
        errorRight = maxRef - refRight; //right sensor error is 16000 - "the value the right sensor sees"
        
        if (refLeft < 5000 && refRight < 5000) //both of the middle sensors see under 5000 so we consider this as a situation where both of the sensor are on white
        {
            if (lastSeenDirection == 1) //left  was the direction the black line was seen last
            {
                MotorDirLeft_Write(0); //left motor drive forwards
                MotorDirRight_Write(0); //right motor drive forwards
                PWM_WriteCompare1(0); //left motor speed (pwm 0-255)
                PWM_WriteCompare2(maxSpeed); //right motor speed to (pwm 0-255)
                CyDelay(driveDelay); //drive with the previous settings for this long
            }
            else //last seen right
            {
                MotorDirLeft_Write(0); //left motor drive forwards
                MotorDirRight_Write(0); //right motor drive forwards
                PWM_WriteCompare1(maxSpeed); //left motor speed (pwm 0-255)
                PWM_WriteCompare2(0); //right motor speed to (pwm 0-255)
                CyDelay(driveDelay); //drive this long
            }
        }
        else //At least one of the middle sensors see something other than white
        {
            if (refLeft == maxRef && refRight == maxRef) //both of the middle sensors are on black
            {
                MotorDirLeft_Write(0); //left motor drive forwards
                MotorDirRight_Write(0); //right motor drive forwards
                PWM_WriteCompare1(maxSpeed); //left motor speed (pwm 0-255)
                PWM_WriteCompare2(maxSpeed); //right motor speed to (pwm 0-255)
                CyDelay(driveDelay); //drive this long
            }
            else if (errorLeft < errorRight) //left sees black, right starts seeing white
            {
                motorSpeed = Kp*errorRight+Kd*(errorRight-lastErrorRight); //calculate the pd controller value
                //motorSpeed maximum is 260000, we need to scale it to 0-255, motorSpeed is proportional to the actual motor speed
                //motorSpeed/260000 = x/255 
                //x = motorSpeed/260000*255
                motorSpeed = motorSpeed/pwmScaler*maxSpeed; //scale the motor speed value to 0-255
                motorSpeedInt = (int) motorSpeed; //cast motorspeed as int
                if (motorSpeed > 255) //if motor speed is over 255 make it 255 (which is the max value)
                {
                    motorSpeed = maxSpeed;
                }
                MotorDirLeft_Write(0); 
                MotorDirRight_Write(0);
                PWM_WriteCompare1(maxSpeed-motorSpeedInt); //left motor speed (pwm 0-255)
                PWM_WriteCompare2(maxSpeed); //right motor speed to (pwm 0-255)
                CyDelay(driveDelay); //drive this long
                lastSeenDirection = 1; 
            }
            else if (errorRight < errorLeft) //right sees black
            {
                motorSpeed = Kp*errorLeft+Kd*(errorLeft-lastErrorLeft);
                motorSpeed = motorSpeed/pwmScaler*255;
                motorSpeedInt = (int) motorSpeed;
                if (motorSpeed > 255)
                {
                    motorSpeed = maxSpeed;
                }
                MotorDirLeft_Write(0); 
                MotorDirRight_Write(0);
                PWM_WriteCompare1(maxSpeed); //left motor speed (pwm 0-255)
                PWM_WriteCompare2(maxSpeed-motorSpeedInt); //right motor speed to (pwm 0-255)
                CyDelay(driveDelay); //drive this long
                lastSeenDirection = 0;
            }
        }
        lastErrorLeft = errorLeft; //previous error = current error
        lastErrorRight = errorRight;
        
        //calculate the amount of black lines seen
        if(ref.l3 > blackLineTreshold && ref.r3 > blackLineTreshold && lastSeenBlackLine == 0) //if the left and right most reflectance sensors see black and we weren't on top of a black line the last time we checked then blacklinecounter++
        {
            timesSeenBlackLine++;
            lastSeenBlackLine = 1; //we are on top of a black line
        }
        else if((ref.l3 < blackLineTreshold || ref.r3 < blackLineTreshold) && lastSeenBlackLine == 1) //if moved away from over a black line
        {
            lastSeenBlackLine = 0; //we are not on top of a black line
        }
        
        if (timesSeenBlackLine == 3) //robot completed the route
        {
            PWM_WriteCompare1(0); //stop the motors
            PWM_WriteCompare2(0);
            break; //break away from the main line following loop
        }
        
        //Battery + LED
        time = GetTicks()/1000; //seconds
        if (time > (60*timesCheckedBattery)) //go here every 60 seconds
        {
            ADC_Battery_StartConvert();
            if(ADC_Battery_IsEndConversion(ADC_Battery_WAIT_FOR_RESULT)) {   // wait for get ADC converted value
                adcresult = ADC_Battery_GetResult16(); // get the ADC value (0 - 4095)
                volts = (float) adcresult/4095*5*1.5; //scale the ADC result value from 0-4095 to 0-5V and multiply it by 1.5 to take into account the voltage reduction in the microcontroller
            }
            timesCheckedBattery++; //variable that tells how many times the battery has been checked. Used for checking the battery every 5 seconds.
        }
        if (volts < 4) //if battery is too low flash the led
        {
            if (ledOn == 0) //if led is off, turn the led on
            {
                BatteryLed_Write(1);
                ledOn = 1;
            }
            else //if led is on turn the led off
            {
                BatteryLed_Write(0);
                ledOn = 0;
            }
            CyDelay(10);
        }
        if (volts >= 4 && ledOn == 1) //if battery is back to over 4 and led was left on, turn it off
        {
            BatteryLed_Write(0);
            ledOn = 0;
        }
    }
    motor_stop();  //stop the motors
}   
#endif

//Line following with PD and if-else hybrid. Also has the old if-else logic in it but commented out.
#if 0
int main()
{
    struct sensors_ ref;
    struct sensors_ dig;
    
    CyGlobalIntEnable; 
    UART_1_Start();
    Systick_Start();
    
    ADC_Battery_Start();        

    int time = 0, timesCheckedBattery = 1, ledOn = 0; //battery variables
    int lastSeenDirection = 0; //last seen direction of the line (0 = left, 1 = right)
    int counter = 0; //counter for printing reflectance sensor variables
    int driveDelay = 8, maxSpeed = 255;
    int timesSeenAllBlack = 1;
    int lastSeenAllBlack = 0;
    //PID hybrid
    int position = 0; //sensor showing the position of our robot, 0 middle, negative on the left, positive on the right
    int error = 0, lastPosition = 0;
    int Kp = 50, Kd = 50;
    //PID
    int turnLeft = 0; // 0 - 255
    int turnRight = 0; // 0 - 255
    
    int16 adcresult =0;
    float volts = 5.0;
  
    reflectance_start();
    CyDelay(20);
    reflectance_set_threshold(9000, 10000, 10000, 10000, 10000, 9000); // set center sensor threshold to 11000 and others to 9000
    
    printf("\nBEEP BOOP\n");

    //BatteryLed_Write(1); // Switch led on 
    BatteryLed_Write(0); // Switch led off 
    //uint8 button;
    //button = SW1_Read(); // read SW1 on pSoC board
    // SW1_Read() returns zero when button is pressed
    // SW1_Read() returns one when button is not pressed
    
    motor_start(); //start the motor
    PWM_WriteCompare1(0); //set left motor speed to 0
    PWM_WriteCompare2(0); //set right motor speed to 0
    
    /*IR_Start();
    
    printf("\n\nIR test\n");
    
    IR_flush(); // clear IR receive buffer
    printf("Buffer cleared\n");
    
    IR_wait(); // wait for IR command*/
    
    for(;;)
    {
        //Battery + LED
        time = GetTicks()/1000; //seconds
        if (time > (10*timesCheckedBattery)) //go here every 10 seconds
        {
            ADC_Battery_StartConvert();
            if(ADC_Battery_IsEndConversion(ADC_Battery_WAIT_FOR_RESULT)) {   // wait for get ADC converted value
                adcresult = ADC_Battery_GetResult16(); // get the ADC value (0 - 4095)
                // convert value to Volts
                // you need to implement the conversion
                
                // Print both ADC results and converted value
                volts = (float) adcresult/4095*5*1.5;
                printf("%d %f V\r\n",adcresult, volts);
            }
            timesCheckedBattery++; //variable that tells how many times the battery has been checked. Used for checking the battery every 5 seconds.
        }
        if (volts < 4) //if battery is too low keep flashing the led
        {
            if (ledOn == 0) //if led is off, turn the led on
            {
                BatteryLed_Write(1);
                ledOn = 1;
            }
            else //if led is on turn the led off
            {
                BatteryLed_Write(0);
                ledOn = 0;
            }
            CyDelay(10);
        }
        if (volts >= 4 && ledOn == 1) //if battery is back to over 4 and led was left on, turn it off
        {
            BatteryLed_Write(0);
            ledOn = 0;
        }
        
        //Line reading with motor control
        // read raw sensor values
        reflectance_read(&ref);
        //printf("%5d %5d %5d %5d %5d %5d\r\n", ref.l3, ref.l2, ref.l1, ref.r1, ref.r2, ref.r3);       // print out each period of reflectance sensors
        //r3 = oikea reuna, 13 = vasen reuna
        // read digital values that are based on threshold. 0 = white, 1 = black
        // when blackness value is over threshold the sensors reads 1, otherwise 0
        reflectance_digital(&dig); //print out 0 or 1 according to results of reflectance period
        //printf("%5d %5d %5d %5d %5d %5d \r\n", dig.l3, dig.l2, dig.l1, dig.r1, dig.r2, dig.r3);        //print out 0 or 1 according to results of reflectance period
        
        //printf("L2 sensori: %5d\n",ref.l2);
        //printf("R2 sensori: %5d\n",ref.r2);
        /*if (ref.l1 > 10000 && ref.l2 > 10000 && ref.l3 > 10000) //90 degree left
        {
            //motor_turn(20,100,2000);
            MotorDirLeft_Write(1);      // set LeftMotor forward mode
            PWM_WriteCompare1(120);     //left motor speed
            MotorDirRight_Write(0);     // set RightMotor backward mode
            PWM_WriteCompare2(120);     //right motor speed
            CyDelay(455);
            lastSeenDirection = 0;
        }
        else if (ref.r1 > 10000 && ref.r2 > 10000 && ref.r3 > 10000) //90 right
        {
            //motor_turn(100,20,2000);
            MotorDirLeft_Write(0);      // set LeftMotor forward mode
            PWM_WriteCompare1(120);     //left motor speed
            MotorDirRight_Write(1);     // set RightMotor backward mode
            PWM_WriteCompare2(120);     //right motor speed
            CyDelay(400);
            lastSeenDirection = 1;
        }*/
        
        /*
        100000
        110000
        010000
        011000
        001000
        001100
        000100
        000110
        000010
        000011
        000001
        */
        if (timesSeenAllBlack == 4)
        {
            break; //go outside of the infinte for loop (to motor_stop();)
        }
        if (lastSeenAllBlack == 0)
        {
            if(dig.l1 == 1 && dig.l2 == 1 && dig.l3 == 1 && dig.r1 == 1 && dig.r2 == 1 && dig.r3 == 1)
            {
                if(timesSeenAllBlack == 1)
                {
                    PWM_WriteCompare1(0); //set left motor to go forwards
                    PWM_WriteCompare2(0); //set right motor to go forwards
                    CyDelay(2000); //wait for 2 seconds
                    motor_forward(255,200);
                }
                timesSeenAllBlack++;
            }
            lastSeenAllBlack = 1;
        }
        if(dig.l3 == 1 && dig.l2 == 0 && dig.l1 == 0 && dig.r1 == 0 && dig.r2 == 0 && dig.r3 == 0)
        {
            //100000
            //Only the left most reflectance sensors sees the line
            /*
            MotorDirLeft_Write(1); //set left motor to go backwards
            MotorDirRight_Write(0);
            PWM_WriteCompare1(20); //left motor speed to 20 (pwm 0-255)
            PWM_WriteCompare2(maxSpeed-20); //right motor speed to maxSpeed-20 (pwm 0-255)
            CyDelay(driveDelay); //drive with the motor speed/direction settings for this delay amount (milliseconds)
            */
            lastSeenDirection = 0;
            lastSeenAllBlack = 0;
            position = -5;
        }
        else if(dig.l3 == 0 && dig.l2 == 0 && dig.l1 == 0 && dig.r1 == 0 && dig.r2 == 0 && dig.r3 == 1)
        {
            //000001
            //only the right mose reflectance sensor sees the line
            /*
            MotorDirLeft_Write(0);
            MotorDirRight_Write(1);
            PWM_WriteCompare1(maxSpeed-20);
            PWM_WriteCompare2(20);
            CyDelay(driveDelay);
            */
            lastSeenDirection = 1;
            lastSeenAllBlack = 0;
            position = 5;
        }
        else if(dig.l3 == 1 && dig.l2 == 1 && dig.l1 == 0 && dig.r1 == 0 && dig.r2 == 0 && dig.r3 == 0)
        {
            //110000
            //2 left most sensors see the line
            /*
            MotorDirLeft_Write(0);
            MotorDirRight_Write(0);
            PWM_WriteCompare1(0);
            PWM_WriteCompare2(maxSpeed-10);
            CyDelay(driveDelay);
            */
            lastSeenDirection = 0;
            lastSeenAllBlack = 0;
            position = -4;
        }
        else if(dig.l3 == 0 && dig.l2 == 0 && dig.l1 == 0 && dig.r1 == 0 && dig.r2 == 1 && dig.r3 == 1)
        {
            //000011
            //2 right most sensors see the line
            /*
            MotorDirLeft_Write(0);
            MotorDirRight_Write(0);
            PWM_WriteCompare1(maxSpeed-10);
            PWM_WriteCompare2(0);
            CyDelay(driveDelay);
            */
            lastSeenDirection = 1;
            lastSeenAllBlack = 0;
            position = 4;
        }
        else if(dig.l3 == 0 && dig.l2 == 1 && dig.l1 == 0 && dig.r1 == 0 && dig.r2 == 0 && dig.r3 == 0)
        {
            //010000
            //Only the sensor that is second from the left sees a line
            /*
            MotorDirLeft_Write(0);
            MotorDirRight_Write(0);
            PWM_WriteCompare1(80);
            PWM_WriteCompare2(maxSpeed);
            CyDelay(driveDelay);
            */
            lastSeenDirection = 0;
            lastSeenAllBlack = 0;
            position = -3;
        }
        else if(dig.l3 == 0 && dig.l2 == 0 && dig.l1 == 0 && dig.r1 == 0 && dig.r2 == 1 && dig.r3 == 0)
        {
            //000010
            /*
            MotorDirLeft_Write(0);
            MotorDirRight_Write(0);
            PWM_WriteCompare1(maxSpeed);
            PWM_WriteCompare2(80);
            CyDelay(driveDelay);
            */
            lastSeenDirection = 1;
            lastSeenAllBlack = 0;
            position = 3;
        }
        else if(dig.l3 == 0 && dig.l2 == 1 && dig.l1 == 1 && dig.r1 == 0 && dig.r2 == 0 && dig.r3 == 0)
        {
            //011000
            /*
            MotorDirLeft_Write(0);
            MotorDirRight_Write(0);
            PWM_WriteCompare1(140);
            PWM_WriteCompare2(maxSpeed);
            CyDelay(driveDelay);
            */
            lastSeenDirection = 0;
            lastSeenAllBlack = 0;
            position = -2;
        }
        else if(dig.l3 == 0 && dig.l2 == 0 && dig.l1 == 0 && dig.r1 == 1 && dig.r2 == 1 && dig.r3 == 0)
        {
            //000110
            /*
            MotorDirLeft_Write(0);
            MotorDirRight_Write(0);
            PWM_WriteCompare1(maxSpeed);
            PWM_WriteCompare2(140);
            CyDelay(driveDelay);
            */
            lastSeenDirection = 1;
            lastSeenAllBlack = 0;
            position = 2;
        }
        else if(dig.l3 == 0 && dig.l2 == 0 && dig.l1 == 1 && dig.r1 == 0 && dig.r2 == 0 && dig.r3 == 0)
        {
            //001000
            /*
            MotorDirLeft_Write(0);
            MotorDirRight_Write(0);
            PWM_WriteCompare1(205);
            PWM_WriteCompare2(maxSpeed);
            CyDelay(driveDelay);
            */
            lastSeenDirection = 0;
            lastSeenAllBlack = 0;
            position = -1;
        }
        else if(dig.l3 == 0 && dig.l2 == 0 && dig.l1 == 0 && dig.r1 == 1 && dig.r2 == 0 && dig.r3 == 0)
        {
            //000100
            /*
            MotorDirLeft_Write(0);
            MotorDirRight_Write(0);
            PWM_WriteCompare1(maxSpeed);
            PWM_WriteCompare2(205);
            CyDelay(driveDelay);
            */
            lastSeenDirection = 1;
            lastSeenAllBlack = 0;
            position = 1;
        }
        else if(dig.l3 == 0 && dig.l2 == 0 && dig.l1 == 1 && dig.r1 == 1 && dig.r2 == 0 && dig.r3 == 0)
        {
            //001100
            //both middle sensors see the line so we go forwards with full speed
            /*
            motor_forward(maxSpeed,driveDelay);
            */
            lastSeenAllBlack = 0;
            position = 0;
        }
        else if (dig.l1 == 0 && dig.l2 == 0 && dig.l3 == 0 && dig.r1 == 0 && dig.r2 == 0 && dig.r3 == 0)
        {
            //we only see white with every sensor
            if (lastSeenDirection == 0) //last seen line was on the left => turn left
            {
                motor_turn(maxSpeed-255,maxSpeed,driveDelay);
            }
            else //last seen line on the right => turn right
            {
                motor_turn(maxSpeed,maxSpeed-255,driveDelay);
            }
            lastSeenAllBlack = 0;
        }
        /*else if (dig.l1 == 1 && dig.l2 == 1 && dig.l3 == 1 && dig.r1 == 1 && dig.r2 == 1 && dig.r3 == 1)
        {
            PWM_WriteCompare1(maxSpeed);
            PWM_WriteCompare2(maxSpeed);
        }*/
        else 
        {
            PWM_WriteCompare1(maxSpeed);
            PWM_WriteCompare2(maxSpeed);
            lastSeenAllBlack = 0;
        }
        
        if (counter == 12)
        {
            //printf("Vasemmat: L3: %5d L2: %5d L1: %5d oik.: R1: %5d R2: %5d R3: %5d \n", ref.l3, ref.l2, ref.l1, ref.r1, ref.r2, ref.r3);
            printf("Vasemmat: %5d %5d %5d oik.: %5d %5d %5d \n", dig.l3, dig.l2, dig.l1, dig.r1, dig.r2, dig.r3);
            printf("%5d %5d %5d %5d %5d %5d\r\n", ref.l3, ref.l2, ref.l1, ref.r1, ref.r2, ref.r3);
            
        
            printf("error: %d\n", error);
            printf("position: %d\n", position);
            printf("Last position: %d\n", lastPosition);
            printf("P: %d\n",(Kp*position));
            printf("D: %d\n",(Kd*(position-lastPosition)));
            counter = 0;
        }
        
        //PID hybrid controller (only PD because I is not needed in this case)
        if (lastPosition < 0)
        {
            if (position < lastPosition) // -5 < -4
            {
                error = (-1)*(Kp*position)+(Kd*(-1)*(position+(-1)*lastPosition));
            }
            else  // -3 > -4
            {
                error = (-1)*(Kp*position)+(Kd*(-1)*(position+(-1)*lastPosition));
            }
        }
        else
        {
            if (position < lastPosition) // 3 < 4
            {
                error = (Kp*position)+(Kd*(position-lastPosition));
            }
            else // 4 > 3
            {
                error = (Kp*position)+(Kd*(position-lastPosition));
            }
        }
        lastPosition = position;
        
        if (error > 255) { //oikealla
            error = 255;
        }
        else if (error < 0) //vasemmalla
        {
            error *= -1;
        }
        
        if (position < 0) //vasemmalla
        {
            MotorDirLeft_Write(0); 
            MotorDirRight_Write(0);
            if (maxSpeed-error < 10)
            {
                MotorDirLeft_Write(1);
                PWM_WriteCompare1(35);
            }
            else if (maxSpeed-error < 60)
            {
                MotorDirLeft_Write(1); //set left motor to go backwards
                PWM_WriteCompare1(15);
            }
            else
            {
                PWM_WriteCompare1(maxSpeed-error); //left motor speed to 20 (pwm 0-255)
            }
            PWM_WriteCompare2(maxSpeed); //right motor speed to maxSpeed-20 (pwm 0-255)
            CyDelay(driveDelay); //drive with the motor speed/direction settings for this delay amount (milliseconds)
            MotorDirLeft_Write(0);
        } 
        else //oikealla tai keskellä
        {
            MotorDirLeft_Write(0); 
            MotorDirRight_Write(0);
            PWM_WriteCompare1(maxSpeed); //left motor speed to 20 (pwm 0-255)
            if (maxSpeed-error < 10)
            {
                MotorDirRight_Write(1);
                PWM_WriteCompare2(35); //right motor speed to maxSpeed-20 (pwm 0-255)
            }
            else if (maxSpeed-error < 60)
            {
                MotorDirRight_Write(1); //set right motor to go backwards
                PWM_WriteCompare2(15);
            }
            else
            {
                PWM_WriteCompare2(maxSpeed-error);
            }
            
            CyDelay(driveDelay); //drive with the motor speed/direction settings for this delay amount (milliseconds)
            MotorDirRight_Write(0);
        }
        
        //PID controller
        
        turnLeft = (18000-ref.l1)/(18000-9000)*255;
        turnRight = (18000-ref.r1)/(18000-9000)*255;
        
        counter++;
    }
    motor_stop();  //stop the motor
 }   
#endif



 //examples after this line

#if 0
// button
int main()
{
    CyGlobalIntEnable; 
    UART_1_Start();
    Systick_Start();
    
    printf("\nBoot\n");

    //BatteryLed_Write(1); // Switch led on 
    BatteryLed_Write(0); // Switch led off 
    
    //uint8 button;
    //button = SW1_Read(); // read SW1 on pSoC board
    // SW1_Read() returns zero when button is pressed
    // SW1_Read() returns one when button is not pressed
    
    bool led = false;
    
    for(;;)
    {
        // toggle led state when button is pressed
        if(SW1_Read() == 0) {
            led = !led;
            BatteryLed_Write(led);
            ShieldLed_Write(led);
            if(led) printf("Led is ON\n");
            else printf("Led is OFF\n");
            Beep(1000, 150);
            while(SW1_Read() == 0) CyDelay(10); // wait while button is being pressed
        }        
    }
 }   
#endif


#if 0
//ultrasonic sensor//
int main()
{
    CyGlobalIntEnable; 
    UART_1_Start();
    Systick_Start();
    Ultra_Start();                          // Ultra Sonic Start function
    while(1) {
        int d = Ultra_GetDistance();
        //If you want to print out the value  
        printf("distance = %d\r\n", d);
        CyDelay(200);
    }
}   
#endif


#if 0
//IR receiver//
int main()
{
    CyGlobalIntEnable; 
    UART_1_Start();
    IR_Start();
    
    uint32_t IR_val; 
    
    printf("\n\nIR test\n");
    
    IR_flush(); // clear IR receive buffer
    printf("Buffer cleared\n");
    
    IR_wait(); // wait for IR command
    printf("IR command received\n");
    
    // print received IR pulses and their lengths
    for(;;)
    {
        if(IR_get(&IR_val)) {
            int l = IR_val & IR_SIGNAL_MASK; // get pulse length
            int b = 0;
            if((IR_val & IR_SIGNAL_HIGH) != 0) b = 1; // get pulse state (0/1)
            printf("%d %d\r\n",b, l);
            //printf("%d %lu\r\n",IR_val & IR_SIGNAL_HIGH ? 1 : 0, (unsigned long) (IR_val & IR_SIGNAL_MASK));
        }
    }    
 }   
#endif


#if 0
//reflectance//
int main()
{
    struct sensors_ ref;
    struct sensors_ dig;

    Systick_Start();

    CyGlobalIntEnable; 
    UART_1_Start();
  
    reflectance_start();
    reflectance_set_threshold(9000, 9000, 11000, 11000, 9000, 9000); // set center sensor threshold to 11000 and others to 9000
    

    for(;;)
    {
        // read raw sensor values
        reflectance_read(&ref);
        printf("%5d %5d %5d %5d %5d %5d\r\n", ref.l3, ref.l2, ref.l1, ref.r1, ref.r2, ref.r3);       // print out each period of reflectance sensors
        //r3 = oikea reuna, 13 = vasen reuna
        // read digital values that are based on threshold. 0 = white, 1 = black
        // when blackness value is over threshold the sensors reads 1, otherwise 0
        reflectance_digital(&dig);      //print out 0 or 1 according to results of reflectance period
        //printf("%5d %5d %5d %5d %5d %5d \r\n", dig.l3, dig.l2, dig.l1, dig.r1, dig.r2, dig.r3);        //print out 0 or 1 according to results of reflectance period
        
        CyDelay(200);
    }
}   
#endif


#if 0
//motor//
int main()
{
    CyGlobalIntEnable; 
    UART_1_Start();

    motor_start();              // motor start

    //motor_forward(255,4000);     // moving forward
    //motor_turn(255,50,1000);     // turn
    //motor_forward(255,4000);
    //motor_turn(50,255,1000);     // turn
    //motor_backward(100,2000);    // movinb backward
    
        motor_forward(50,4000);/*
        motor_turn(255,1,700);
        motor_forward(255,2000);
        motor_turn(1,255,700);
    
        
        motor_forward(122,1000);
        motor_turn(122,1,1400);
        motor_forward(122,1000);
        motor_turn(1,122,1400);
        
        
        motor_forward(255,1000);
        motor_backward(255,500);
        motor_backward(150,500);
        motor_backward(75,500);
        
        
        
        motor_forward(255,2000);
        motor_turn(255,1,700);
        motor_forward(255,2000);
        motor_turn(1,255,700);
    
        
        motor_forward(122,1000);
        motor_turn(122,1,1400);
        motor_forward(122,1000);
        motor_turn(1,122,1400);
        
        
        motor_forward(255,1000);
        motor_backward(255,500);
        motor_backward(150,500);
        motor_backward(75,500);
    */
       
    motor_stop();               // motor stop
    
    for(;;)
    {

        
        
    }
}
#endif


/* [] END OF FILE */
