# Heart-Rate-Moniter

/**
 * @file main.cpp
 * @authors Malavika Venkatanarayanan <mv2290@nyu.edu> and Rheya Vithalani <rrv7869@nyu.edu>
 * @brief 
 * @version 0.1
 * @date 2022-05-16
 */

/**
 * ------------------------------------Equipments Used------------------------------------- 
 * 1. Sensor Used : A pressure sensor to translate pressure into digital data.  
 * We are using the Honeywell MPRLS0300YG00001BB. This is an I2C pressure sensor capable of measuring 0-300mmHg
 * Datasheet : MPRLS0025PA00001A-Honeywell.pdf
 * 2. Pediatric Blood Pressure Measurement Kit cosisting of 
 *  A. 0-300 mmHG Aneroid Gauge 
 *  B. Durable Nylon Cuff
 *  C. Standard Air release valve
 * 
 * 3. Transition Air Tubing
 * 
 * 4. F-F wire jumpers
 */

#define I2C_SDA PC_9
#define I2C_SCL PA_8

Sensor Address = (0x18 << 1);

Sensor Address Read Address = 0x31; 

Sensor Command Array = { 0xAA, 0x00, 0x00 }; 

Lowest possible Pressure = 0.0; 

Highest possible Pressure = 300.0; 

Lowest sensor Output = 419430.4; 

float sensor_Output_Max = 3774873.6; 
