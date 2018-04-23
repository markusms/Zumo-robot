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

#if 1
//battery level//
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
    int driveDelay = 5, maxSpeed = 255;

    //PD controller
    int Kp = 19, Kd = 35;
    int errorLeft = 0, errorRight = 0, error = 0;
    int lastErrorLeft = 0, lastErrorRight = 0, lastError = 0;
    int turnLeft = 0; 
    int turnRight = 0; 
    int motorSpeedInt = 0;
    double motorSpeed = 0;
    int refLeft = 16000, refRight = 16000; //when the robot is in the middle of the line both sensors see a bit of white because they aren't in the middle of the line
    int maxRef = 16000;
    int pwmScaler = 260000;
    
    int16 adcresult = 0;
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
    
    /*
    IR_Start();
    IR_flush(); // clear IR receive buffer
    IR_wait(); // wait for IR command
    */
    
    for(;;)
    {
        //Line reading with motor control
        // read raw sensor values
        reflectance_read(&ref);
        //printf("%5d %5d %5d %5d %5d %5d\r\n", ref.l3, ref.l2, ref.l1, ref.r1, ref.r2, ref.r3);       // print out each period of reflectance sensors
        //r3 = oikea reuna, 13 = vasen reuna
        // read digital values that are based on threshold. 0 = white, 1 = black
        // when blackness value is over threshold the sensors reads 1, otherwise 0
        reflectance_digital(&dig); //print out 0 or 1 according to results of reflectance period
        //printf("%5d %5d %5d %5d %5d %5d \r\n", dig.l3, dig.l2, dig.l1, dig.r1, dig.r2, dig.r3);        //print out 0 or 1 according to results of reflectance period
        
        /*
        if (counter == 12)
        {
            //printf("Vasemmat: L3: %5d L2: %5d L1: %5d oik.: R1: %5d R2: %5d R3: %5d \n", ref.l3, ref.l2, ref.l1, ref.r1, ref.r2, ref.r3);
            //printf("Vasemmat: %5d %5d %5d oik.: %5d %5d %5d \n", dig.l3, dig.l2, dig.l1, dig.r1, dig.r2, dig.r3);
            printf("%5d %5d %5d %5d %5d %5d\r\n", ref.l3, ref.l2, ref.l1, ref.r1, ref.r2, ref.r3);
            counter = 0;
        }
        */
        
        counter++;
        
        //PID controller (PD)
        //turnLeft = (18000-ref.l1)/(18000-9000)*255;
        //turnRight = (18000-ref.r1)/(18000-9000)*255;

        
        if (ref.l1 > maxRef)
        {
           refLeft = maxRef;
        }
        else
        {
            refLeft = ref.l1;
        }
        if (ref.r1 > maxRef)
        {
            refRight = maxRef;
        }
        else
        {
            refRight = ref.r1;
        }
        errorLeft = maxRef - refLeft;
        errorRight = maxRef - refRight;
        
        if (refLeft < 5000 && refRight < 5000) //both see white
        {
            if (lastSeenDirection == 1) //left was seen last
            {
                MotorDirLeft_Write(0); 
                MotorDirRight_Write(0);
                PWM_WriteCompare1(0); //left motor speed (pwm 0-255)
                PWM_WriteCompare2(maxSpeed); //right motor speed to (pwm 0-255)
                CyDelay(driveDelay); //drive this long
            }
            else //last seen right
            {
                MotorDirLeft_Write(0); 
                MotorDirRight_Write(0);
                PWM_WriteCompare1(maxSpeed); //left motor speed (pwm 0-255)
                PWM_WriteCompare2(0); //right motor speed to (pwm 0-255)
                CyDelay(driveDelay); //drive this long
            }
        }
        else
        {
            if (refLeft == maxRef && refRight == maxRef) //both are on black
            {
                MotorDirLeft_Write(0); 
                MotorDirRight_Write(0);
                PWM_WriteCompare1(maxSpeed); //left motor speed (pwm 0-255)
                PWM_WriteCompare2(maxSpeed); //right motor speed to (pwm 0-255)
                CyDelay(driveDelay);
            }
            else if (errorLeft < errorRight) //left sees black, right starts seeing white
            {
                motorSpeed = Kp*errorRight+Kd*(errorRight-lastErrorRight);
                /*printf("errorRight: %d\n", errorRight);
                printf("lastErrorRight: %d\n", lastErrorRight);
                printf("motorSpeed: %lf\n", motorSpeed);*/
                //if (ref.l1 = 3000) { motorSpeed = 430000), we need to scale it to 0-255
                //motorSpeed/430000 = x/255 
                //x = motorSpeed/430000*255
                motorSpeed = motorSpeed/pwmScaler*255;
                motorSpeedInt = (int) motorSpeed;
                if (motorSpeed > 255)
                {
                    motorSpeed = maxSpeed;
                }
                /*if (motorSpeedInt < 0)
                {
                    MotorDirLeft_Write(1); //left motor backwards
                    MotorDirRight_Write(0); //right motor forward
                    PWM_WriteCompare1(motorSpeedInt*(-1)); //left motor speed (pwm 0-255)
                    PWM_WriteCompare2(maxSpeed); //right motor speed to (pwm 0-255)
                    CyDelay(driveDelay); //drive this long
                }*/
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
                //printf("motorSpeedScaledLEFT: %d\n", 255-motorSpeedInt);
                if (motorSpeed > 255)
                {
                    motorSpeed = maxSpeed;
                }
               /* if (motorSpeedInt < 0)
                {
                    MotorDirLeft_Write(0); //left motor forward
                    MotorDirRight_Write(1); //right motor backwards
                    PWM_WriteCompare1(maxSpeed); //left motor speed (pwm 0-255)
                    PWM_WriteCompare2(motorSpeedInt*(-1)); //right motor speed to (pwm 0-255)
                    CyDelay(driveDelay); //drive this long
                }*/
                MotorDirLeft_Write(0); 
                MotorDirRight_Write(0);
                PWM_WriteCompare1(maxSpeed); //left motor speed (pwm 0-255)
                PWM_WriteCompare2(maxSpeed-motorSpeedInt); //right motor speed to (pwm 0-255)
                CyDelay(driveDelay); //drive this long
                lastSeenDirection = 0;
            }
        }
        /*printf("kd error %d\n", errorLeft-lastErrorLeft);
        CyDelay(400);*/
        lastErrorLeft = errorLeft;
        lastErrorRight = errorRight;
        
        //Battery + LED
        /*
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
        */
    }
    motor_stop();  //stop the motor
 }   
#endif

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
    
       
    motor_stop();               // motor stop
    
    for(;;)
    {

        
        
    }
}
#endif


/* [] END OF FILE */
