/*

this simple examples shows how you can save data to a memory chip, and print the results to the serial monitor

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
uint8_t rPoint = 0, rA0Volts = 0, rA1Volts = 0;
uint8_t rD2State = 0, rCharTest = 0;

// data to store measurements
uint16_t b0Bits = 0, b1Bits = 0;
float A0Volts = 0, A1Volts = 0;
uint8_t D2State = 0;
uint32_t Counter = 0, oldTime = 0;
uint32_t Point = 0;

int16_t recordsetID = 0;
int32_t Ret = 0, i = 0;

// create the database driver object
DBase SSD(SSD_PIN);

void setup() {

  Serial.begin(115200);

  while (!Serial) {}

  // initialize the database object
  Ret = SSD.init();


  // check if the chip is found and supported
  if (!Ret) {
    Serial.println("Chip may be invalid");
  }

  // start adding fields
  // most popular datatypes are supported
  // except String--but you should not be using these anyway :)

  // char data is justed passed in (as char arrays are already pointers)
  rCharTest = SSD.addField("Name", RecordName, sizeof(RecordName));

  // numeric data must be passed in by ref using the &
  // this will allow the library to simply point to your data so saving record
  // automatically is aware of your data
  rPoint = SSD.addField("Point", &Point);
  rA0Volts = SSD.addField("A0 Volts", &A0Volts);
  rA1Volts = SSD.addField("A1 Volts", &A1Volts);
  rD2State = SSD.addField("Pin 2", &D2State);

  // mandatory is to find the last valid record
  // fields must be created first so the library knows the record length for searching
  // if you forget this line, it will be called upon the first saveRecord() call
  // doing it this way allows you to get record counts, id, etc.
  Ret = SSD.findLastRecord();

  // a very negative limititaion of these memory chips is you can only write to a byte in an unsaved state
  // also, this library does not accomodate saving data in different sessions with different field counts
  // here's how to erase the chip

  //Serial.println("erasing chip");
  //SSD.eraseAll();
  //Serial.println("chip erased.");

  // process the return... maybe you want to reuse and id or such, here's where that can be done
  if (Ret == CHIP_NEW) {
    Serial.println("Unused chip, no records");
  } else if (Ret == CHIP_FULL) {
    Serial.println("Chip is full.");
  } else {
    Serial.print("Records found: "); Serial.println(Ret);
  }

  LogData();

  PrintRecords();

  Serial.println("Demo complete");
}

void loop() {
}

// basic data logging loop
void LogData() {

  // use specifies points and duration (below)
  for (i = 0; i < 50; i++) {
    oldTime = millis();

    // increment data point
    Point++;

    // get some readings and process
    b0Bits = analogRead(SENSOR1_PIN);
    b1Bits = analogRead(SENSOR2_PIN);
    D2State = digitalRead(SENSOR3_PIN);

    A0Volts = (b0Bits * 3.3) / 1024.0;
    A1Volts = (b1Bits * 3.3) / 1024.0;

    Serial.print("Saving to RecordSetID: ");
    Serial.print(recordsetID);
    Serial.print(", Point: ");
    Serial.println(Point);

    // Serial.print("creating string ");
    // sprintf(CharTestData, "Record %04d", (int) Point);
    // careful the max char len is controlled by MAXDATACHARLEN in the .h file
    sprintf(RecordName, "Rec. %4d", (int)Point);
    //Serial.println(CharTestData);

    // add a new record
    SSD.addRecord();
    // save.. remember the field data are pointers that lib has direct access to
    SSD.saveRecord();

    // wait until time is up
    while (millis() - oldTime < 10) {
    }
  }
}

// dump data to the serial monitor
void PrintRecords() {

  uint32_t i = 0;
  uint32_t cr = SSD.getCurrentRecord();

  // print field headers
  // again field list is 1 based
  for (i = 1; i <= SSD.getFieldCount(); i++) {
    Serial.print(SSD.getFieldName(i));
    Serial.print(", ");
  }

  Serial.println(" ");
  Serial.print("Last Record: ");
  Serial.println(SSD.getLastRecord());
  Serial.println(" ");

  // print records
  // records are 1 based hence must start at 1
  for (i = 1; i <= SSD.getLastRecord(); i++) {
    SSD.gotoRecord(i);
    Serial.print(i);
    Serial.print(", ");
    Serial.print(SSD.getCharField(rCharTest));
    Serial.print(", ");
    Serial.print(SSD.getField(Point, rPoint));
    Serial.print(", ");
    Serial.print(SSD.getField(A0Volts, rA0Volts), 5);
    Serial.print(", ");
    Serial.print(SSD.getField(A1Volts, rA1Volts), 5);
    Serial.print(", ");
    Serial.print(SSD.getField(D2State, rD2State));
    Serial.println(" ");
  }

  Serial.println("Record printing complete.");

  // restore current record
  SSD.gotoRecord(cr);
}

// end of example
