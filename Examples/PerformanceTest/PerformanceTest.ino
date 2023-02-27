/*

this examples shows performance with this library

Testing Initialization...Initialization time [us]: 50047
Testing chip erase...Chip erase time [us]: 21629510
Testing field creation...Time to add 6 fields [us]: 4, [us/byte]: 1
Testing record add...Time to add and save 1 record [us/record]: 766, [us/byte]: 32
Time to find last valid record [us]: 382
Testing 100 record add...Time to add and save 100 records [us/record]: 756
Time to goto a record and read a field [us]: 47

#define SPEED_WRITE 25000000
#define SPEED_READ  25000000

*/

// include the database library
#include "DBase.h"

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
DBase SSD(SSD_PIN);

void setup() {

  Serial.begin(115200);

  while (!Serial) {}

  Serial.println("*************************************");
  Serial.println("*                                   *");
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


  // time initialization
  Serial.print("Testing Initialization...");
  Timer = micros();
  // initialize the database object
  Ret = SSD.init();
  Serial.print("Initialization time [us]: ");
  Serial.println(micros() - Timer);
  delay(100);

  // time chip erase creation
  Serial.print("Testing chip erase...");
  Timer = micros();
  SSD.eraseAll();
  Serial.print("Chip erase time [us]: ");
  Serial.println(micros() - Timer);
  delay(1000);

  // time field creation
  Serial.print("Testing field creation...");
  Timer = micros();
  rCharTest = SSD.addField("Name", RecordName, sizeof(RecordName));
  rID = SSD.addField("Set ID", &recordsetID);
  rPoint = SSD.addField("Point", &Point);
  rA0Volts = SSD.addField("A0 Volts", &A0Volts);
  rA1Volts = SSD.addField("A1 Volts", &A1Volts);
  rD2State = SSD.addField("Pin 2", &D2State);

  Serial.print("Time to add:");
  Serial.print(SSD.getFieldCount());
  Serial.print(" fields [us]: ");
  Serial.print((micros() - Timer) / SSD.getFieldCount());
  Serial.print(", [us/byte]: ");
  Serial.println((micros() - Timer) / SSD.getRecordLength());
  delay(100);

  // time to add and save a record
  // poplulate some dummy data
  Serial.print("Testing record add...");
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
  Serial.print("Time to add and save 1 record [us/record]: ");
  Serial.print((micros() - Timer));
  Serial.print(", [us/byte]: ");
  Serial.println((micros() - Timer) / SSD.getRecordLength());
  delay(1000);

  // time to find last record
  Timer = micros();
  Ret = SSD.findLastRecord();
  Serial.print("Time to find last valid record [us]: ");
  Serial.println(micros() - Timer);
  delay(1000);

  // Time to add 100 records
  Serial.print("Testing 100 record add...");
  Timer = micros();
  for (i = 0; i < 100; i++) {

    SSD.addRecord();
    SSD.saveRecord();
  }
  Serial.print("Time to add and save 100 records [us/record]: ");
  Serial.println((micros() - Timer) / 100);
  delay(1000);

  // Time to add goto a record and read a field
  Timer = micros();
  SSD.gotoRecord(50);
  SSD.getField(A0Volts, rA0Volts);
  Serial.print("Time to goto a record and read a field [us]: ");
  Serial.println((micros() - Timer));
}

void loop() {
}

// end of example
