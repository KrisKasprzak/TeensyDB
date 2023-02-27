/*

this examples shows how you can save data to a memory chip, print the saved data, get chip stats, and even
dump the data to and SD card. The save method used here has a field called recordsetID. This ID will let you
write to the same recordset or a new one. Classic examples are 1) continue writing to the same recordset
after a power up or 2) always write to a new recordset.

Make sure the serial monitor is active as there is a simple menu to run throuh some example functions.

While this demo saves data to the chip, there is an option to write all that data to and SD card

*/

// include the database library
#include "DBase.h"

// include the sd card library (this is optional, if you don't need to write to an SD card, delete related code)
#include "SdFat.h"

// some SPI chip select pins
#define SSD_PIN 6
#define SD_PIN 2

#define SENSOR1_PIN A0
#define SENSOR2_PIN A1
#define SENSOR3_PIN 7

// careful the max char len is controlled by MAXDATACHARLEN in the .h file
char RecordName[MAXDATACHARLEN];

// required ID's to store the created field ID's
uint8_t rID = 0, rPoint = 0, rA0Volts = 0, rA1Volts = 0;
uint8_t rD2State = 0, rCharTest = 0;

// data to store measurements
uint16_t b0Bits = 0, b1Bits = 0;
float A0Volts = 0, A1Volts = 0;
uint8_t D2State = 0;
uint32_t Counter = 0, oldTime = 0;
uint32_t Point = 0;

int16_t recordsetID = 0, i;
int32_t Ret = 0;

// create the database driver object
DBase SSD(SSD_PIN);

void setup() {

  Serial.begin(115200);

  while (!Serial) {}

  // initialize the database object
  Ret = SSD.init();

  // check if the chip is found and supported
  if (Ret) {
    Serial.println("Chip found");
    ChipStats();
  } else {
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
  rID = SSD.addField("Set ID", &recordsetID);
  rPoint = SSD.addField("Point", &Point);
  rA0Volts = SSD.addField("A0 Volts", &A0Volts);
  rA1Volts = SSD.addField("A1 Volts", &A1Volts);
  rD2State = SSD.addField("Pin 2", &D2State);

  // mandatory is to find the last valid record
  // fields must be created first so the library knows the record length for searching
  // if you forget this line, it will be called upon the first saveRecord() call
  // doing it this way allows you to get record counts, id, etc.
  Ret = SSD.findLastRecord();

  // process the return... maybe you want to reuse and id or such, here's where that can be done
  if (Ret == CHIP_NEW) {
    Serial.println("Unused chip, no records");
  } else if (Ret == CHIP_FULL) {
    Serial.println("Chip is full.");
  } else {
    Serial.print("Records found: ");
    Serial.println(Ret);
  }

  // draw a cute user menu
  DrawMenu();
}

void loop() {

  // simple loop, check for serial monitor key press
  CheckKeyPress();
}

// function to draw menu...nothing fancy
void DrawMenu() {

  Serial.println("____________ DBase Demo Main Menu ____________");
  Serial.println();
  Serial.println("Enter(S) for list chip stats");
  Serial.println("Enter(F) for field list");
  Serial.println("Enter(E) to erase chip");
  Serial.println("Enter(P) to print records to Serial monitor");
  Serial.println("Enter(N) for logging 1000 data points to a new dataset");
  Serial.println("Enter(C) for logging 1000 data points to the current dataset");
  Serial.println("Enter(D) for download data to and SD card");
  Serial.println("Enter(Q) to dump byte address to the monitor");
  Serial.println();
  Serial.println();
  Serial.println("Enter(M) for main menu");
  Serial.println();
}

// proces any serail input...nothing fancy
void CheckKeyPress() {

  if (Serial.available()) {

    char key;
    key = Serial.read();
    switch (key) {
      case 'M':
        // redraw menu
        DrawMenu();
        break;
      case 'S':
        // get and display chip stats
        ChipStats();
        DrawMenu();
        break;
      case 'F':
        // list input fields
        ListDataStats();
        DrawMenu();
        break;
      case 'E':
        // erase entire chip
        Serial.println("This may take a few minutes...");
        SSD.eraseAll();
        recordsetID = 1;
        Serial.println("Erase complete");
        DrawMenu();
        break;
      case 'P':
        // print entire list of records to the serial monitor
        PrintRecords();
        DrawMenu();
        break;
      case 'N':
        // option to log data using a new recordset id
        Serial.println("Logging...");
        // this is a new data set so we want to get the last recordset id and bump it up one
        // goto the last record and get the data set id
        // the increment
        SSD.gotoLastRecord();
        recordsetID = SSD.getField(recordsetID, rID);
        recordsetID++;

        // new dataset, restart point
        Point = 0;

        LogData();

        DrawMenu();
        break;
      case 'C':
        // option to log data to the last known recordset id
        Serial.println("Logging...");
        // this is appending the current dataset so we want to get the last recordset id and use it
        // but we get teh last point and bump it up one
        // goto the last record and get the data set id
        // the increment
        SSD.gotoLastRecord();
        recordsetID = SSD.getField(recordsetID, rID);

        // now get the last point and increment
        Point = SSD.getField(Point, rPoint);
        LogData();

        DrawMenu();

        break;
      case 'Q':
        // more for debugging, but you can see the byte value at each address
        // noting a float occupies 4 bytes so 3.1415 will be the 4 byte equivalent 208-15-73-64
        SSD.dumpBytes();
        DrawMenu();
        break;
      case 'D':
        DownloadData();
        DrawMenu();
        break;
    }
  }
}

void ListDataStats() {
  uint8_t i;
  // rip through all fields and print field parameters
  // noting that fields are 1 based
  for (i = 1; i <= SSD.getFieldCount(); i++) {
    Serial.print("Field number: ");
    Serial.print(i);
    Serial.print(", Name: ");
    Serial.print(SSD.getFieldName(i));
    Serial.print(", type: ");

    if (SSD.getFieldDataType(i) == DT_U8) {
      Serial.print("uint8_t");
    } else if (SSD.getFieldDataType(i) == DT_INT) {
      Serial.print("int");
    } else if (SSD.getFieldDataType(i) == DT_I16) {
      Serial.print("int16_t");
    } else if (SSD.getFieldDataType(i) == DT_U16) {
      Serial.print("uint16_t");
    } else if (SSD.getFieldDataType(i) == DT_I32) {
      Serial.print("int32_t");
    } else if (SSD.getFieldDataType(i) == DT_U32) {
      Serial.print("uint32_t");
    } else if (SSD.getFieldDataType(i) == DT_FLOAT) {
      Serial.print("float");
    } else if (SSD.getFieldDataType(i) == DT_DOUBLE) {
      Serial.print("double");
    } else if (SSD.getFieldDataType(i) == DT_CHAR) {
      Serial.print("char");
    }

    Serial.print(", Field length (bytes): ");
    Serial.println(SSD.getFieldLength(i));
  }

  Serial.print("Record length: ");
  Serial.println(SSD.getRecordLength());
  Serial.print("Field count: ");
  Serial.println(SSD.getFieldCount());
  Serial.print("Record length: ");
  Serial.println(SSD.getRecordLength());
  Serial.print("Total Records ");
  Serial.println(SSD.getLastRecord());
  Serial.print("Address or last real record: ");
  Serial.println(SSD.getLastRecord());
  Serial.print("Last RecordSetID: ");
  SSD.getLastRecord();
  Serial.println(SSD.getField(recordsetID, rID));
  Serial.println();
  Serial.println();
}

// basic data logging loop
void LogData() {

  // use specifies points and duration (below)
  for (i = 0; i < 1000; i++) {
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

// shows some of the methods to get chip statistics
void ChipStats() {

  Serial.println("Chip Stats");
  Serial.println("\r\r\r");
  Serial.print("Chip ID: ");
  Serial.println(SSD.getChipJEDEC());
  Serial.print("Used Space (b): ");
  Serial.println(SSD.getUsedSpace());
  Serial.print("Total Space (b): ");
  Serial.println(SSD.getTotalSpace());


  Serial.println("\r\r");
  Serial.println("Enter(M) for main menu\r\r");
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
    Serial.print(SSD.getField(recordsetID, rID));
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

// dump data to and SD card
// optional, get the SD card example working first
// to debug wiring, pins etc.

void DownloadData() {

  uint32_t i = 0;
  uint32_t cr = SSD.getCurrentRecord();
  bool SDOK = false;

  // create the objects in here as opposed to global
  SdFat sd;
  SdFile file;

  // i have a cheap SD card, so I back the data rate way donw
  if (!sd.begin(SD_PIN, SD_SCK_MHZ(20))) {
    Serial.println("Cannot start SD card");
    return;
  }

  // something you can do is create a new file for each recordset, assuming you are implementing
  // a record set approach
  // here, we'll just dump everything to the same file
  SDOK = file.open("Data.txt", FILE_WRITE);

  Serial.println("Writing to: Data.txt");

  if (SDOK) {

    SSD.gotoRecord(1);

    // print field headers
    for (i = 1; i <= SSD.getFieldCount(); i++) {
      file.print(SSD.getFieldName(i));
      if (i == SSD.getFieldCount()) {
        file.println();
      } else {
        file.print(", ");
      }
    }
    // print records
    for (i = 1; i <= SSD.getLastRecord(); i++) {
      SSD.gotoRecord(i);

      file.print(SSD.getCharField(rCharTest));
      file.print(", ");
      file.print(SSD.getField(recordsetID, rID));
      file.print(", ");
      file.print(SSD.getField(Point, rPoint));
      file.print(", ");
      file.print(SSD.getField(A0Volts, rA0Volts), 5);
      file.print(", ");
      file.print(SSD.getField(A1Volts, rA1Volts), 5);
      file.print(", ");
      file.print(SSD.getField(D2State, rD2State));
      file.println(" ");
    }

    Serial.println("Record printing complete.");
    file.close();
    SSD.gotoRecord(cr);

  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("Error opening the data file.");
    return;
  }
}


// end of example
