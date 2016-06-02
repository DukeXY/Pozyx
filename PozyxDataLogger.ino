// Please read the ready-to-localize tuturial together with this example.
// https://www.pozyx.io/Documentation/Tutorials/ready_to_localize
#include <Pozyx.h>
#include <Pozyx_definitions.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

////////////////////////////////////////////////
////////////////// PARAMETERS //////////////////
////////////////////////////////////////////////
int currentLog=0;
int pushButton=8;
bool bButtonPushed=false;

uint8_t num_anchors = 4;                                    // the number of anchors
uint16_t anchors[4] = {0x6037, 0x6042, 0x605C, 0x6020};     // the network id of the anchors: change these to the network ids of your anchors.
int32_t heights[4] = {730, 430, 1270, 0};              // anchor z-coordinates in mm
boolean bProcessing = false;                                // set this to true to output data for the processing sketch         

// only required for manual anchor calibration. Please change this to the coordinates measured for the anchors
int32_t anchors_x[4] = {0, 10000, 1000, 9000};              // anchor x-coorindates in mm
int32_t anchors_y[4] = {0, 0, 7000, 8000};                  // anchor y-coordinates in mm

boolean bRemote = false;                  // boolean to indicate if we want to read sensor data from the attached pozyx shield (value 0) or from a remote pozyx device (value 1)
uint16_t destination_id = 0x6000;     // the network id of the other pozyx device: fill in the network id of the other device
uint32_t last_millis;                 // used to compute the measurement interval in milliseconds 

String dataFileName;
////////////////////////////////////////////////

void setup(){
  pinMode(4, OUTPUT); digitalWrite(4, LOW);
  pinMode(10, OUTPUT); digitalWrite(10, HIGH);
  pinMode(pushButton, INPUT); // button for new log

  Serial.begin(115200);
  if (!SD.begin(4)) {
    Serial.println(F("ERROR: Card failed, or not present"));
    return;
  }
  else {Serial.println(F("Card initialized."));}
  Serial.println();
  delay(1000);
  Serial.println(Pozyx.begin());
  
  if(Pozyx.begin() == POZYX_FAILURE){
    Serial.println(F("ERROR: Unable to connect to POZYX shield"));
    Serial.println(F("Reset required"));
    delay(100);
    abort();
  }
  
  Serial.println(F("----------POZYX POSITIONING V1.0----------"));
  Serial.println(F("NOTES:"));
  Serial.println(F("- No parameters required."));
  Serial.println();
  Serial.println(F("- System will auto start calibration"));
  Serial.println();
  Serial.println(F("- System will auto start positioning"));
  Serial.println(F("----------POZYX POSITIONING V1.0----------"));
  Serial.println();
  Serial.println(F("Performing auto anchor calibration:"));

  // clear all previous devices in the device list
  Pozyx.clearDevices();
     
  int status = Pozyx.doAnchorCalibration(POZYX_2_5D, 50, num_anchors, anchors, heights);
  if (status != POZYX_SUCCESS){
    Serial.println(status);
    Serial.println(F("ERROR: calibration"));
    Serial.println(F("Reset required"));
    delay(100);
    abort();
  }
  
  // if the automatic anchor calibration is unsuccessful, try manually setting the anchor coordinates.
  // fot this, you must update the arrays anchors_x, anchors_y and heights above
  // comment out the doAnchorCalibration block and the if-statement above if you are using manual mode
  //SetAnchorsManual();

  printCalibrationResult();
  delay(3000);
  
  Serial.println(F("Starting positioning and recording data: "));
  last_millis = millis();
}
int cnt=0;
void loop(){
  cnt++;
  if (cnt==101) delay(1000000);
  int16_t sensor_data[24];
  uint8_t calib_status = 0; 
  int i, dt;
  int bButtonState = digitalRead(pushButton);
  if ((bButtonState == 1) && (!bButtonPushed)) {bButtonPushed = true; currentLog++;}
  if ((bButtonState == 0) && (bButtonPushed)) {bButtonPushed = false;}
  dataFileName = String(currentLog) + ".TXT";

  File data=SD.open(dataFileName,FILE_WRITE);
  
  // print the measurement interval  
  dt = millis() - last_millis;
  data.print(dt, DEC);
  data.print(" ");
  Serial.print(dt, DEC);
  
  uint32_t pressure = ((uint32_t)sensor_data[0]) + (((uint32_t)sensor_data[1])<<16);
  data.print(pressure);
  data.print(" ");
  Serial.print(",");
  Serial.print(pressure);

  // print out all remaining sensors
  for(i=2; i<24; i++){
    data.print(sensor_data[i]);
    data.print(" ");
    Serial.print(",");
    Serial.print(sensor_data[i]);
  }
  data.close();

  coordinates_t position;  
  int status = Pozyx.doPositioning(&position, POZYX_3D);
  
  if (status == POZYX_SUCCESS)
  {
    // print out the result
    printCoordinates(position);
  }

    
  if(bRemote == true)
  {
    // remotely read the sensor data
    int status = Pozyx.remoteRegRead(destination_id, POZYX_PRESSURE, (uint8_t*)&sensor_data, 24*sizeof(int16_t));
    if(status != POZYX_SUCCESS){  
      return;
    }
      
  }else
  {
    // wait until this device gives an interrupt
    if (Pozyx.waitForFlag(POZYX_INT_STATUS_IMU, 10))
    {
      // we received an interrupt from pozyx telling us new IMU data is ready, now let's read it!            
      Pozyx.regRead(POZYX_PRESSURE, (uint8_t*)&sensor_data, 24*sizeof(int16_t)); 
             
      // also read out the calibration status
      Pozyx.regRead(POZYX_CALIB_STATUS, &calib_status, 1);  
    }else{
      // we didn't receive an interrupt
      uint8_t interrupt_status = 0;
      Pozyx.regRead(POZYX_INT_STATUS, &interrupt_status, 1);
    
      return;  
    }
  }
          
  // print out the presure (this is not an int16 but rather an uint32
}

// function to print the coordinates to the serial monitor
void printCoordinates(coordinates_t coor){
  Serial.print(coor.x);
  Serial.print(" ");
  Serial.print(coor.y);
  Serial.print(" ");
  Serial.print(coor.z);
  Serial.println(); 
  
  File data=SD.open(dataFileName,FILE_WRITE);
  
  data.print(coor.x);
  data.print(" ");
  data.print(coor.y);
  data.print(" ");
  data.print(coor.z);
  data.println();
  
  data.close(); 
}


// print out the anchor coordinates (also required for the processing sketch)
void printCalibrationResult(){
  uint8_t list_size;
  int status;

  status = Pozyx.getDeviceListSize(&list_size);
  Serial.print("list size: ");
  Serial.println(status*list_size);
  
  if(list_size == 0){
    Serial.println("Calibration failed.");
    Serial.println(Pozyx.getSystemError());
    return;
  }
  
  uint16_t device_ids[list_size];
  status &= Pozyx.getDeviceIds(device_ids,list_size);
  
  Serial.println(F("Calibration result:"));
  Serial.print(F("Anchors found: "));
  Serial.println(list_size);
  
  coordinates_t anchor_coor;
  for(int i=0; i<list_size; i++)
  {
    
    Serial.print("ANCHOR,");
    Serial.print("0x");
    Serial.print(device_ids[i], HEX);
    Serial.print(",");    
    status = Pozyx.getDeviceCoordinates(device_ids[i], &anchor_coor);
    Serial.print(anchor_coor.x);
    Serial.print(",");
    Serial.print(anchor_coor.y);
    Serial.print(",");
    Serial.println(anchor_coor.z);
    
  }    
}

// function to manually set the anchor coordinates
void SetAnchorsManual(){
 
 int i=0;
 for(i=0; i<num_anchors; i++){
   device_coordinates_t anchor;
   anchor.network_id = anchors[i];
   anchor.flag = 0x1; 
   anchor.pos.x = anchors_x[i];
   anchor.pos.y = anchors_y[i];
   anchor.pos.z = heights[i];
   Pozyx.addDevice(anchor);
 }
 
}
