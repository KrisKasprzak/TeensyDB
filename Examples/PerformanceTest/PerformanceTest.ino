/*

this examples shows performance with this library

TeensyDB Library performance tests: 

Microchip SST25F040C with Teensy 4.0
1. Time to initialize the chip...[us]: 50005
2. Time to chip erase (will be approx 30 sec.)... [us]: 231491
3. Time to add fields...Adding : 6 records, 34 bytes each:  [us]: 2
4. Time to add and save 1 record...Time [us/record]: 3111, [us/byte]: 91
5. Time to find the last record (worst case scenario)... [us]: 3
6. Time to add and save 100 records...Total Time [us]: 311475, time [us/record]: 3114, [us/byte]: 91.61
7. Time to add goto a record and read a field... [us]: 14
8. Time to read 100 records (NORMAL SPEED)... [us/record]: 13.86, [us/byte]: 0.41
9. Time to read 100 records (FAST SPEED)... [us/record]: 11.80, [us/byte]: 0.35

Winbond 25Q64JVSIQ with Teensy 4.0

1. Time to initialize the chip... [us]: 50005
2. Time to chip erase (will be approx 30 sec.)...[us]: 21534893
3. Time to add fields... Adding : 6 records, 34 bytes each: Time [us]: 3
4. Time to add and save 1 record... [us/record]: 816, [us/byte]: 24
5. Time to find the last record (worst case scenario)... [us]: 3
6. Time to add and save 100 records...Total Time [us]: 81385, time [us/record]: 813, [us/byte]: 23.94
7. Time to add goto a record and read a field... [us]: 14
8. Time to read 100 records (NORMAL SPEED)... [us/record]: 13.83, [us/byte]: 0.41
9. Time to read 100 records (FAST SPEED)...[us/record]: 11.79, [us/byte]: 0.35

*/

// include the database library
#include "TeensyDB.h"

// some SPI chip select pins
#define SSD_PIN 6
#define SENSOR1_PIN A0
#define SENSOR2_PIN A1
#define SENSOR3_PIN 7

// careful the max char len is controlled by MAXDATACHARLEN in the .h file
char RecordName[MAXDATACHARLEN];

// required ID's to store the created field ID's
uint8_t rID = 0, rPoint = 0, rA0Volts = 0, rA1Volts = 0;
uint8_t rD2State = 0, rCharTest = 0;

// data to store measurements
uint8_t recordsetID = 0;
uint16_t b0Bits = 0, b1Bits = 0;
float A0Volts = 0, A1Volts = 0;
uint8_t D2State = 0;
uint32_t Counter = 0, oldTime = 0;
uint32_t Point = 0;

int16_t i = 0;
int32_t Ret = 0;
int32_t Timer = 0;
// create the database driver object
TeensyDB SSD(SSD_PIN);

void setup() {

  Serial.begin(115200);

  while (!Serial) {}
  
  Serial.println("*************************************");
  Serial.println("*                                   *");
  Serial.println("* TeensyDB Performance Tester       *");
  Serial.println("* Running this performance test     *");
  Serial.println("* will require you erase your chip  *");
  Serial.println("* press Y to proceed                *");
  Serial.println("*                                   *");
  Serial.println("*************************************");

  Serial.println();
  Serial.println();


  while (!Serial.available()) {
  }

  if (Serial.read() == 'Y') {

  } else {

    while (1) {}
  }

  Serial.println("TeensyDB Library performance tests");
  Serial.println("__________________________________________________________________");
  Serial.println();
  Serial.println();

  // time initialization
  Serial.println("Time to initialize the chip...");
  Timer = micros();
  // initialize the database object
  Ret = SSD.init();
  Serial.print("Time [us]: ");
  Serial.println(micros() - Timer);
  delay(100);
  Serial.println();

  // time chip erase creation
  Serial.println("Time to chip erase (will be approx 30 sec.)...");
  Timer = micros();
  // SSD.eraseAll();
  Serial.print("Time [us]: ");
  Serial.println(micros() - Timer);
  delay(1000);
  Serial.println();

  // time field creation
  Serial.println("Time to add fields...");
  Timer = micros();
  rCharTest = SSD.addField(RecordName, sizeof(RecordName));
  rID = SSD.addField(&recordsetID);
  rPoint = SSD.addField(&Point);
  rA0Volts = SSD.addField(&A0Volts);
  rA1Volts = SSD.addField(&A1Volts);
  rD2State = SSD.addField(&D2State);

  Serial.print("Addding : ");
  Serial.print(SSD.getFieldCount());
  Serial.print(" records, ");
  Serial.println(SSD.getRecordLength());
  Serial.print(" bytes each: ");
  Serial.print("Time [us]: ");
  Serial.println((micros() - Timer));
  delay(1000);
  Serial.println();

  // time to add and save a record
  // poplulate some dummy data
  Serial.println("Time to add and save 1 record...");
  strcpy(RecordName, "Test");
  recordsetID = 1;
  Point = 2;
  A0Volts = 12.34;
  A1Volts = 56.78;
  D2State = 1;
  SSD.gotoRecord(0);
  Timer = micros();
  SSD.addRecord();
  SSD.saveRecord();
  Serial.print("Time [us/record]: ");
  Serial.print((micros() - Timer));
  Serial.print(", [us/byte]: ");
  Serial.println((micros() - Timer) / SSD.getRecordLength());
  delay(1000);
  Serial.println();

  // time to add and save a record
  // poplulate some dummy data
  Serial.println("Time to find the last record (worst case scenario)...");
  // time to find last record
  Timer = micros();
  Ret = SSD.findFirstWritableRecord();
  Serial.print("Time [us]: ");
  Serial.println(micros() - Timer);
  delay(1000);
  Serial.println();

  // Time to add 100 records
  Serial.println("Time to add and save 5000 records...");
  Timer = micros();
  for (i = 0; i < 5000; i++) {
    SSD.addRecord();
    SSD.saveRecord();
  }
  Serial.print("Total Time [us]: ");
  Serial.print(micros() - Timer);
  Serial.print(", time [us/record]: ");
  Serial.print((micros() - Timer) / 100);
  Serial.print(", [us/byte]: ");
  Serial.println((float)(micros() - Timer) / (float)(100 * SSD.getRecordLength()));
  delay(1000);
  Serial.println();

  Serial.println("Time to add goto a record and read a field...");
  Timer = micros();
  SSD.gotoRecord(50);
  SSD.getField(A0Volts, rA0Volts);
  Serial.print("Time [us]: ");
  Serial.println((micros() - Timer));
  delay(1000);
  Serial.println();


  Serial.println("Time to read 100 records (NORMAL SPEED)...");
  SSD.setReadSpeed(READ_NORMAL);  // this is the default
  Timer = micros();

  for (i = 1; i <= 100; i++) {
    SSD.gotoRecord(i);
    SSD.getField(Point, rPoint);
  }
  Serial.print("Time [us/record]: ");
  Serial.print((micros() - Timer) / 100.0);
  Serial.print(", [us/byte]: ");
  Serial.println((micros() - Timer) / (100.0 * SSD.getRecordLength()));
  Serial.println();
  delay(1000);

  Serial.println("Time to read 100 records (FAST SPEED)...");
  SSD.setReadSpeed(READ_FAST);  // this is the default
  Timer = micros();
  for (i = 1; i <= 100; i++) {
    SSD.gotoRecord(i);
    SSD.getField(Point, rPoint);
  }
  Serial.print("Time [us/record]: ");
  Serial.print((micros() - Timer) / 100.0);
  Serial.print(", [us/byte]: ");
  Serial.println((micros() - Timer) / (100.0 * SSD.getRecordLength()));
  delay(1000);
  Serial.println();
  Serial.print("DBase Library performance tests complete...");
}

void loop() {
}

// end of example
