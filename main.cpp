/**
 * @file main.cpp
 * @authors Malavika Venkatanarayanan <mv2290@nyu.edu> and Rheya Vithalani <rrv7869@nyu.edu>
 * @brief Blood Pressure Montoring System 
 * @version 0.1
 * @date 2022-05-16
 * 
 * @copyright Copyright (c) 2022
 * 
 */
 
#include "mbed.h" 
#include "stdio.h"

/**
* Define the Pins inorder to communicate with Honeywell Sensor through I2C (Inter-Integrated Circuit) Protocol
*/
#define I2C_SDA PC_9
#define I2C_SCL PA_8
 
//::::::::::::::::::::::::::::::::::All Timer related variable::::::::::::::::::::::::::::::   

//Timer Object
Timer timerVal;
//Time array to store the time values which will be plotted on the X axis 
float timerDataArray[500];

//::::::::::::::::::::::::::::All Sensor related variables:::::::::::::::::::::::::::::::::::::

//I2C Object for Honeywell I2C (PinName sda, PinName scl)
I2C i2cForHoneywell(I2C_SDA, I2C_SCL);
//The address of the Honeywell sensor to send during I2C communication
int honeywellSensorAddress = (0x18 << 1);
//The address to send for the read command of the Honeywell sensor
int honeywellSensorReadAddress = 0x31;
//The commands to send in order for I2C write communication
const char honewellSensorCommand[] = { 0xAA, 0x00, 0x00 };
//The 4 byte result returned by the sensor after reading from the Honeywell sensor
char sensorReading[4] = {0};
//The output value calculated using sensorReading
float pressureOutput = 0.0;
//The minimum output given by the sensor (refer the datasheet)
float sensorMinimumPressureReading = 419430.4;
//The maximum output given by the sensor (refer the datasheet)
float sensorMaximumPressureReading = 3774873.6;
//Sensor result status
char sensorReadingStatusBit = '0';

//:::::::::::::::::::::::::All pressure calculation variables::::::::::::::::::::::::::::::::::::
 
//This is the lowest value in mm Hg that our pressure sensor can read
float lowestPressure = 0.0;
//Stores the pressure calculated from the sensor
float pressure = 0.0;
//This is the highest value in mm hg that our pressure sensor can read
float highestPressure = 300.0;
//The previous pressure value that was calculated
float previousPressureVal = 0.0;

//Check if pressure is increasing or decreasing
bool isPressureIncreasing = true;
bool isPressureDecreasing = false;

//Loop counter variable for pressure array , it stores the total values recorded from the sensor between 150 mm hg and 30 mm hg
int pressureCounter = 0;
// Array to store all the pressure values
float pressureArrayValues[500];
//Counts the number of positive slopes throughout the graph
float heartRateCount = 0;
//Store the mean arterial pressure 
float meanArterialPressureWA = 0.0;
//Store the mean arterial pressure 
float meanArterialPressureSlope = 0.0;
//Store Pulse Pressure 
float pulsePressure = 0.0;

//:::::::::::::::::::::::::All slope calculation variables::::::::::::::::::::::::::::::::::::

//Array to store al/ the slopes between various pressure data points. We initialize the slopes to 1
float pressureSlope[500] = {1.0};
//The value of the maximum positive slope
float maxSlope=0;
//The index of the maximum positive slope which is there in the slope array
int MaxIndexPositiveSlope = 0;
//Stores the index of the Systolic Pressure which is there in the pressure array
int systolicPressureSlopeIndex = 0;
//Stores the index of the Diastolic Pressure which is there in the pressure array
int diastolic_pressureSlopeIndex = 0;
 
//:::::::::::::::::::::::::Deflation rate variables:::::::::::::::::::::::::::
 
//The deflation rate message
char *deflationRateMessage;
 
void measurePressureValuesFromTheHoneywellSensor()
{
    while(1)
    {
        //printf (" Pressure value at the start of calculate pressure = %.2f \n", pressure);
        //As and when the pressure goes above 150 for the first time as we pump, then isPressureIncreasing shall be false as we do 
        //not want to take this value into calculations.
        if(pressure > 150)
            isPressureIncreasing = false;

        //now that we are releasing the valve and the pressure goes below than 151 mm Hg at that time isPressureDecreasing will
        //be set to true
        //isPressureIncreasing is false when we finished pumping (i.e After 150)
        if (pressure < 151 && !isPressureIncreasing)
            isPressureDecreasing = true;

        //As soon as the pressure goes below 30 and isPressureDecreasing is set to true that means
        //we now have to break out of the whole loop and show final readings
        if (pressure < 30 and isPressureDecreasing)
            break;

        //Write the commands 0xAA, 0x00 and 0x00 at the given sensor address
        i2cForHoneywell.write(honeywellSensorAddress, honewellSensorCommand, 3);

        //Introduced wait time here after writing through i2c to ensure writing is complete 
        wait_us(5000);

        //printf (" Pressure Sensor result status Before read= %d \n", sensorReadingStatusBit); //only print pressure readings
        //Reads the status byte using read (int address, char *data, int length, bool repeated=false)
        i2cForHoneywell.read(honeywellSensorReadAddress, &sensorReadingStatusBit, 1);

        //printf (" Pressure Sensor result status After read = %d \n", sensorReadingStatusBit);
        //printf (" Pressure Sensor result status After read and with 0x40= %x \n", (sensorReadingStatusBit & 0x40) >> 6 );
        
        //Reading the 6th bit in status byte and checking if its 1
        if(((sensorReadingStatusBit & 0x40) >> 6) == 0x1)
        {
            //printf (" While True \n");
            i2cForHoneywell.read(honeywellSensorReadAddress, &sensorReadingStatusBit, 1);
            //We are introducing wait after reading through i2c
            wait_us(5000);
        }

        //Read the sensor output and storing it in 4 bytes in the array sensorReading
        i2cForHoneywell.read(honeywellSensorAddress, sensorReading, 4);

        //printf (" Pressure sensorReading = %d \n", sensorReading[3]); 
        //printf (" Pressure sensorReading = %d \n", sensorReading[2]);
        //printf (" Pressure sensorReading = %d \n", sensorReading[1]); 

        //Generate the 24-bit output from the sensor using bit shifting and then we type cast if to float 
        pressureOutput = (float)((sensorReading[1] << 16) | (sensorReading[2] << 8) | (sensorReading[3]));

        //The first byte is ignored as it is the status byte and we collect the other 3 bytes
        //printf (" Pressure pressureOutput = %f \n", pressureOutput); 
        //printf (" Pressure sensorReading = %d \n", sensorReading[3]); 
        //printf (" Pressure sensorReading = %d \n", sensorReading[2]<<8); 
        //printf (" Pressure sensorReading = %d \n", sensorReading[1]<<16); 
        
        //Calcuate the pressure using the formula referring the datasheet 
        pressure = (((pressureOutput - sensorMinimumPressureReading) * (highestPressure - lowestPressure)) / (sensorMaximumPressureReading - sensorMinimumPressureReading)) + lowestPressure;

        //printf ("\nPressure value  = %f \n", pressure);
    
        //If the difference between the consecutive pressure values is less than 3.0 mmHg/sec, the deflation rate is too slow
        if((previousPressureVal - pressure) < 3.0)
        {
            deflationRateMessage = "Deflation Rate too slow, Please make it fast";
        }

        //If the difference between the consecutive pressure values is greater than 5.0 mmHg/sec, the deflation rate is too fast
        else if ((previousPressureVal - pressure) > 5.0)
        {
            deflationRateMessage = "Deflation Rate too fast, Please make it slow";
        }

        //If deflation rate is between 3.0 mmHg/sec and 5.0 mmHg/sec, it is Okay since we have been asked to keep reducing by 4.0 mmHg/sec
        else
        {
            deflationRateMessage = "Deflation Rate Okay, continue";
        }

        //Read the time value using ticker object "timerVal"
        int time_ms = timerVal.elapsed_time().count();

        //If the isPressureDecreasing is true , we are deflating and going below 151mmHg
        if(isPressureDecreasing)
        {
            printf ("\nTimeStamp : %d | Pressure : %f \n", time_ms/1000, pressure);
            //The current calculated pressure is saved into the pressure array
            pressureArrayValues[pressureCounter] = pressure;
            //The time in seconds is saved in the time array
            timerDataArray[pressureCounter] = time_ms/1000;
            //increment the counter value
            pressureCounter++;
        }

        //If the pressure is pumped beyond 151 then start showing deflation rate remarks
        else if(isPressureIncreasing == false)
        {
            printf ("\nTimeStamp : %d | Pressure : %f | Deflation Comment : %s \n", time_ms/1000, pressure, deflationRateMessage);
        }
        //The increasing pressure when we pump the cuff
        else
        {   
            printf ("\nTimeStamp : %d | Pressure : %f\n", time_ms/1000, pressure);
        }

        //Save the current pressure value into previousPressureVal before calculating the new pressure value
        previousPressureVal = pressure;

        //Wait time before taking the next sample
        wait_us(10000);

    }

    //Stop the timer after all the pressure readings and calculations are over
    timerVal.stop();

    //loop for calculating slopes
    for(int i=1;i<pressureCounter;i++)
    {
        //The consecutive pressure differences
        float consecutivePressureDifference = pressureArrayValues[i] - pressureArrayValues[i-1];
        //The consecutive time differences
        float consecutiveTimeDifference=(timerDataArray[i]-timerDataArray[i-1]);
        //Ensure that the time difference not zero 
        //This prevents getting an infinite slope
        if(consecutiveTimeDifference != 0.000000 )
        //Slope array will be calcuated by dividing the change in Pressure with change in time
        pressureSlope[i-1]= (consecutivePressureDifference/consecutiveTimeDifference);
        //printf("Pressure diff, Time Diff, Max diff: %.2f %.2f %.2f\n\r", consecutivePressureDifference, consecutiveTimeDifference, pressureSlope[i-1]);
    }

    //loop for finding the maximum positive slope 
    for(int j=0; j<pressureCounter; j++)
    {
        //If the current slope in the array is greater than our current max positive slope
        if(pressureSlope[j] > maxSlope)
        {
            //Save the new higher value in max_Positive Slope
            maxSlope = pressureSlope[j];
            //add 1 to The index of this slope value to use it in pressure array because we want the higher pressure reading used to calculate the postive slope

            MaxIndexPositiveSlope = j+1;           
        }
    }
 
    printf("::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                       The final data as the pressure drops to 30mmHg                               ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
    
    printf("\nMaximum Pressure Value: is %f\n", pressureArrayValues[MaxIndexPositiveSlope]);

    //Need to print Data for Graphs  
    printf("\n::::::::::::::::::::::::::::::::: Pressure Value on x axis ::::::::::::::::::::::::::::::::::::::::::::\n");
    
    for(int i = 0; i<50; i++) {
    
        printf("%f", pressureArrayValues[i]);
    
    }
    
    printf("\n:::::::::::::::::::::::::::::::: Time Value on x axis :::::::::::::::::::::::::::::::::::::::::::::::::\n");
    
    for(int i = 0; i<50; i++) {
    
        printf("%f", timerDataArray[i]);
    
    }

 }
 
void evaluateSystolicPressure()
{

    //The Systolic pressure slope minimum threshold is calculated by multiplying the maximum slop by 0.5
    float sysSlopeMinThreshold = 0.5 * maxSlope;
    //The difference between slope readings
    float diffInSlope = 0.0;
    //The smallest difference in slope readings. We initialize it with a large value (INT32_MAX)
    float minDiffInSlope=INT32_MAX;

    //Run the loop through the slope array from the start till we reach a value lesser than the index of the slope of the MAP.
    for(int k=0; k < MaxIndexPositiveSlope - 1; k++)
    {
        //If slope is positive && if the slope is less than our systolic threshold value
        if((pressureSlope[k]>=0.0) && (pressureSlope[k] < sysSlopeMinThreshold))
        {
            //The difference between our threshold value and the current slope reading
            diffInSlope = sysSlopeMinThreshold - pressureSlope[k];

            //Iff the calculated difference is less than minimum difference in slope
            if(diffInSlope < minDiffInSlope)
            {
                //The minimum difference in slope 
                minDiffInSlope=diffInSlope;
                //The index for the value in pressure array
                systolicPressureSlopeIndex=k+1; 
            }
        }
    }
  //print the corresponding pressure reading from the pressure array as "Systolic Pressure"
  //printf("\nCalculated Systolic values from oscillations : %.2f \n",  pressureArrayValues[systolicPressureSlopeIndex]);
  printf("\n:::::::::::::::::::::::::::  Calculated Systolic values from oscillations  :::::::::::::::::::::::::::::\n");
  printf("::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
  printf("::                                                                                                    ::\n");
  printf("::                                                                                                    ::\n");
  printf("::                                                                                                    ::\n");
  printf("::                                                                                                    ::\n");
  printf("::%-60s | %-240s%-80s", "            Name              ", "                                          Value","                     ::\n");
  printf("::%-60s | %-60f%-80s","      Systolic values          ",pressureArrayValues[systolicPressureSlopeIndex],"::\n");
  printf("::                                                                                                    ::\n");
  printf("::                                                                                                    ::\n");
  printf("::                                                                                                    ::\n");
  printf("::                                                                                                    ::\n");
  printf("::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
}

void evaluateDiastolicPressure()
{
  //Diastolic pressure slope minimum threshold is calculated by multiplying the maximum slope with 0.8
  float diaSlopeMinThreshold = 0.8 * maxSlope; 
  //The difference in slope readings
  float diffInSlope_dia=0.0; 
  //The smallest difference in slope readings. We initialize it with a very large value using int32_max
  float minDiffInSlope_dia=INT32_MAX; 
  
  //Loop through the slope array from the index post MAP untill we reach a value lesser than the last slope in the slope array
  for(int l=MaxIndexPositiveSlope+1; l < pressureCounter; l++)
   {
      //check first if slope is positive && is the slope less than our diastolic threshold value
      if((pressureSlope[l]>=0.0) && (pressureSlope[l] < diaSlopeMinThreshold))
      {
          //calculate the difference between our threshold value and the current slope reading
          diffInSlope_dia = diaSlopeMinThreshold - pressureSlope[l];
          //check if the calculated difference is less than minimum difference in slope
          if(diffInSlope_dia < minDiffInSlope_dia)
          {
              //then store minimum difference in slope as this calculated difference
              minDiffInSlope_dia=diffInSlope_dia;
              //store the corresponding index for the value in pressure array
              diastolic_pressureSlopeIndex=l+1;
            
          }
      }
   }

    //print the corresponding pressure reading from the pressure array as "Diastolic Pressure"
    //printf("\nCalculated Diastolic values from oscillations: %.2f \n",  pressureArrayValues[diastolic_pressureSlopeIndex]);
    printf("\n::::::::::::::::::::::::::  Calculated Diastolic values from oscillations  ::::::::::::::::::::::::::::\n");
    printf("::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::%-60s | %-120s%-80s", "            Name", "                                                      Value","                      ::\n");
    printf("::%-60s | %-60f%-80s","      Diastolic        ",pressureArrayValues[diastolic_pressureSlopeIndex],"     ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");

 }
 
void evaluateHeartRate()
{
    //Loop through the pressures and get the heart rate count
    for(int m = systolicPressureSlopeIndex-1; m < diastolic_pressureSlopeIndex; m++)
    {
        //check for all positive slopes greater than 0.0
        if(pressureSlope[m] > 0.0)
            {
                //count these positive slopes
                heartRateCount++;
            }  
    }

    //printf("Heart Rate Count : %.2f", heartRateCount);
    //printf("Heart Rate Count : %.2f", (timerDataArray[diastolic_pressureSlopeIndex]/1000 - timerDataArray[systolicPressureSlopeIndex]/1000));
    //printf("\nHeart Rate Val : %.2f", (heartRateCount) / (timerDataArray[diastolic_pressureSlopeIndex] - timerDataArray[systolicPressureSlopeIndex])* 60.0f);
    
    int heart_Rate = (int)(((heartRateCount) / (timerDataArray[diastolic_pressureSlopeIndex]/1000 - timerDataArray[systolicPressureSlopeIndex]/1000))* 60.0f);
    //calculate the heart rate by dividing by the time and then multiplying by 60
    //printf("\nCalculated Heart Rate values from oscillations: %d beats per minute\n",heart_Rate);
    printf("\n::::::::::::::::::::::  Calculated Heart Rate values from oscillations  ::::::::::::::::::::::::::::::::\n");
    printf("::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::%-60s | %-120s%-80s", "            Name            ", "               Value                    ","                            ::\n");
    printf("::%-60s | %-d%-80s","      Heart Rate                                       ",                   heart_Rate,"                   ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
 
}
 
void evaluatePulsePressure()
{

    //Pulse pressure is calculated as the difference between the Systolic and the Diastolic values
    pulsePressure = pressureArrayValues[systolicPressureSlopeIndex] - pressureArrayValues[diastolic_pressureSlopeIndex];

    printf("\n::::::::::::::::::::::  Calculated Pulse pressure from oscillations  ::::::::::::::::::::::::::::::::\n");
    printf("::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::%-60s | %-120s%-80s", "            Name            ", "                                     Value                          ","::\n");
    printf("::%-60s | %-60.2f%-80s","      Pulse Pressure          ",pulsePressure                            ,"                ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
 
}
 
void evaluateMeanArterialPressureUsingWeightedAverageMethod()
{
 
    //MAP = 1/3 Systolic Value + 2/3 Diastolic Value
    //printf("Systolic Pressure ---------- %.2f\n",pressureArrayValues[systolicPressureSlopeIndex]);
    //printf("Diastolic Pressure ---------- %.2f\n",pressureArrayValues[diastolic_pressureSlopeIndex]);
    
    meanArterialPressureWA =  (0.33) * pressureArrayValues[systolicPressureSlopeIndex] + (0.67) * pressureArrayValues[diastolic_pressureSlopeIndex];
    //printf("\nMean Arterial Pressure(MAP) using Weigted Average Method is : %.2f\n", meanArterialPressureWA);
    printf("\n::::::::::::::::::::::  Mean Arterial Pressure Using Weighted Average  ::::::::::::::::::::::::::::::::\n");
    printf("::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::%-60s | %-120s%-80s", "            Name            ", "                                     Value","                         ::\n");
    printf("::%-60s | %-60.2f%-80s","      Mean Arterial Pressure ",meanArterialPressureWA     ,"        ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
 
}
 
void evaluateMeanArterialPressureUsingSlopeMethod()
{
 
    //MAP is taken as the value when the slope is maximum
    meanArterialPressureSlope =  pressureArrayValues[MaxIndexPositiveSlope];
    //printf("\nMean Arterial Pressure(MAP) using Slope Method is : %.2f\n", meanArterialPressureSlope);
    printf("\n:::::::::::::::::::::::::::::  Mean Arterial Pressure Using Slope :::::::::::::::::::::::::::::::::::::\n");
    printf("::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::%-60s | %-120s%-80s", "            Name            ", "                                     Value","                ::\n");
    printf("::%-60s | %-60f%-80s","      Mean Arterial Pressure",meanArterialPressureSlope                    ,"       ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
 
}


int main()
{
    //Start the timer
    timerVal.start();
    
    //Waits 10000 microseconds 
    wait_us(10000);

    printf("::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                       BLOOD PRESSURE MONITORING SYSTEM                             ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::                                                                                                    ::\n");
    printf("::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");   
    
    //Waits 10000 microseconds  
    wait_us(10000);

    printf("\n:::::::::::::::::::::::::::::::::::INSTRUCTIONS:::::::::::::::::::::::::::::::::::\n");
    printf("Put on the cuff2.While measuring the pressure, increase the cuff pressure to 150mmHg3.\n");
    printf("While continuously measuring the pressure, open the pressure relief valve, causing the pressure to reduce about 4mmHg/sec.\n"); 
    printf("The system shall provide notices if the release rate is too fast or too slow\n");
    
    //Waits 10000 microseconds 
    wait_us(10000);

    //To measure the pressure values from the HoneyWell Sensor and Evaluate the Deflation rate
    /**
     * @brief The 24 bit values read from the sensor are stored. The first byte is ignored as it is the status byte. 
     * The calculated pressure value is used to check the deflation rate.
    The deflation rate is printed in the terminal. The timer stops once all the pressure calculations are over. 
    The slope of the pressure is also calculated as follows :

                        slope=(pressure_Difference / time_Difference) 
    The difference between two consecutive values is calculated.
    If it is <3 mm Hg, the deflation rate is too slow and it is printed in the terminal.
    If it is >5 mm Hg, the deflation rate is too fast and it is printed in the terminal.
    If the values are between 3 mm Hg and 5 mm Hg, the deflation rate is ideal
    This helps us to properly release the valve for pressure calculation.
     * 
     */
    measurePressureValuesFromTheHoneywellSensor();

    //Evaluation for Systolic Pressure 
    /**
     * @brief the value of 0.5* maximum peak value is taken for systolic calculation. 
     * The range from that to the initial value is taken in this instance and using the slope values stored earlier, the systolic slope threshold value is 
     * calculated. The closest slope value is  calculated using the smallest difference between current slope reading and sysSlopeMinThreshold. 
     * The index of the closest slope is used to calculate pressure.The loop is run  through the slope array from the start till we reach a value lesser 
     * than the index of the slope of the MAP. Threshold value and the current slope reading.The Difference  between threshold value and the slope value is 
     * calculated and if it’s less than minimum difference in slope then it is stored as the calculated difference and is used to print the systolic value.
     * 
     */
    evaluateSystolicPressure();

    //Evaluation for Diastolic Pressure 
    /**
     * @brief Diastolic calculation.
    0.8 times the Amplitude of peak of MAP is taken as the minimum value for threshold. 
     * All the values from this to the end are taken for diastolic calculation. 
    The same principle for systolic calculation is used here.
     */
    evaluateDiastolicPressure();

    //Evaluation of the Heart Rate
    /**
     * @brief A count is set to count all the positive slopes above 0 and between the systolic and diastolic pressure indices. 
    It is then divided by the time difference corresponding to the indices and multiplied by 60 to give heart beats per minute.
    The typical value of heart rate in humans= 60 to 100 beats per minute 
     * 
     */
    evaluateHeartRate();
    
    //Evaluation of the Pulse Pressure
    /**
     * @brief Pulse pressure is taken as the difference between Systolic and diastolic pressures. It is calculated as follows:
                                                   PP=SBP - DBP
     * 
     */
    evaluatePulsePressure();
    
    //Evaluation of the Mean Arterial Pressure Using Weighted Average Method
    /**
     * @brief  MAP = 1/3 * SBP + 2/3 * DBP
     * 
     */
    evaluateMeanArterialPressureUsingWeightedAverageMethod();

    //Evaluation of the Mean Arterial Pressure Using Slope Method
    /**
     * @brief Using this the maximum value of the slope is calculated.The maximum positive value of the slope is required for the MAP 
     * [Mean Arterial Pressure]estimation. The Mean Arterial Pressure is the global maximum value.
    In Theory, This MAP calculator (Mean Arterial Pressure calculator) finds the average arterial blood pressure during a single cardiac cycle.

     * 
     */
    evaluateMeanArterialPressureUsingSlopeMethod();
 
}