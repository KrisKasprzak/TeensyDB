/*

example for a winbond Winbond 25q64jvsiq chip (64 mbit SPI chip)

this examples shows how you can save data to a memory chip, print the saved data, get chip stats, and even
dump the data to and SD card. The save method used here has a field called recordsetID. This ID will let you
write to the same recordset or a new one. Classic examples are 1) continue writing to the same recordset
after a power up or 2) always write to a new recordset.

Make sure the serial monitor is active as there is a simple menu to run throuh some example functions.

While this demo saves data to the chip, there is an option to write all that data to and SD card. Erasing a chip
before using any of the examples is mandatory since the library cannot determine any existing record size or fields
on the chip

pin connections

flash chip  MCU
CE          6
SO/SIO1     12 (MISO)
WP          3v3
VSS         GND
VDD         3v3
HOLD        3v3
SCK         13 (SCK)
SI/SIO0     11 (MOSI)

*/

// include the database library
#include "TeensyDB.h"

// include the sd card library (this is optional, if you don't need to write to an SD card, delete related code)
#include "SdFat.h"

// some SPI chip select pins
#define SSD_PIN 6
#define SD_PIN 10
#define SENSOR1_PIN A0
#define SENSOR2_PIN A1
#define SENSOR3_PIN A2
#define SENSOR4_PIN A3
#define SENSOR7_PIN 7

#define REC_TO_WRITE 10

// careful the max char len is controlled by MAXDATACHARLEN in the .h file
char RecordName[MAXDATACHARLEN];

// required ID's to store the created field ID's
uint8_t rID = 0, rPoint = 0, rA0Volts = 0, rA1Volts = 0, rA2Volts = 0, rA3Volts = 0;
uint8_t rD2State = 0, rRecordName = 0;

// data to store measurements
uint32_t b0Bits = 0, b1Bits = 0, b2Bits = 0, b3Bits = 0;

byte D2State = 0;
uint32_t Counter = 0;

int Point = 0;
float A0Volts = 0.0;
double A1Volts = 0.0, A2Volts = 0.0, A3Volts = 0.0;
uint8_t recordsetID = 0;

int32_t Ret = 0;
uint32_t i = 0;

uint32_t StartTime = 0, EndTime = 0;

// create the database driver object
TeensyDB SSD(SSD_PIN);

// create the objects in here as opposed to global
SdFat sd;
SdFile file;

void setup() {

  Serial.begin(115200);

  while (!Serial) {
    Serial.println("wait for serial");
    delay(1000);
  }
  // Step 1
  // initialize the database object
  Ret = SSD.init();

  // Step 2
  // start adding fields
  // most popular datatypes are supported
  // except String--but you should not be using these anyway :)

  // adding fields non chars
  // rID = SSD.addField(&Data); // pass by reference as library looks directly at the var
  // adding char fields, data is just passed in (as char arrays are already pointers)
  // rCharTest = SSD.addField(Data, sizeof(Data));

  // numeric data must be passed in by ref using the &
  // this will allow the library to simply point to your data so saving record
  // automatically is aware of your data
  rID = SSD.addField(&recordsetID);
  rRecordName = SSD.addField(RecordName, sizeof(RecordName));
  rPoint = SSD.addField(&Point);
  rA0Volts = SSD.addField(&A0Volts);
  rA1Volts = SSD.addField(&A1Volts);
  rA2Volts = SSD.addField(&A2Volts);
  rA3Volts = SSD.addField(&A3Volts);
  rD2State = SSD.addField(&D2State);

  // Step 3
  // mandatory is to find the last valid record
  // fields must be created first so the library knows the record length for searching
  // if you forget this line, it will be called upon the first saveRecord() call
  // doing it this way allows you to get record counts, id, etc.
  Ret = SSD.findFirstWritableRecord();

  // process the return
  if (Ret > 0) {
    Serial.print(Ret);
    Serial.println(" Records found, ready for use");
  } else if (Ret == CHIP_NEW) {
    Serial.println("Unused chip, no records, ready for use");
  } else if (Ret == CHIP_FULL) {
    Serial.println("Chip is full. records cannot be written, erase chip to use");
  } else if (Ret == CHIP_INVALID) {
    Serial.print("Chip is not supported");
    Serial.println(Ret);
  } else if (Ret == CHIP_FORCE_RESTART) {
    Serial.print("First writable record cannot be found erase chip to use");
  } else if (Ret == NO_FIELDS) {
    Serial.print("No fields initialized, create fields first");
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

  Serial.println("____________ TeensyDB Demo Main Menu ____________");
  Serial.println();
  Serial.println("Enter(S) for list chip stats");
  Serial.println("Enter(F) for field list");
  Serial.println("Enter(E) to erase chip");
  Serial.println("Enter(R) to erase sector 0 (dangerous as fillowing sectors may have data)");
  Serial.println("Enter(B) to erase small block 1 (dangerous as fillowing sectors may have data)");
  Serial.print("Enter(P) to write database to serial monitor. Records found: ");
  Serial.println(SSD.getLastRecord());
  Serial.print("Enter(N) for logging ");
  Serial.print(REC_TO_WRITE);
  Serial.println(" data points to a new dataset");
  Serial.print("Enter(C) for logging ");
  Serial.print(REC_TO_WRITE);
  Serial.println(" data points to the current dataset");
  Serial.println("Enter(D) for download data to and SD card");
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
    switch (toupper(key)) {
      case 'M' :
        // redraw menu
        DrawMenu();
        break;
      case 'S' :
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
      case 'R':
        SSD.eraseSector(0);
        recordsetID = 1;
        Serial.println("Erase complete");
        DrawMenu();
        break;
      case 'B':
        SSD.eraseSmallBlock(1);
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
        Point = 0;

        StartTime = micros();
        LogData();
        EndTime = micros();
        Serial.print(REC_TO_WRITE);
        Serial.print(" records written in: ");
        Serial.print(EndTime - StartTime);
        Serial.println(" uS");
        Serial.print("Performance is: ");
        Serial.print(((REC_TO_WRITE * 1000 * 1000 * SSD.getRecordLength()) / (EndTime - StartTime)));
        Serial.println(" bytes/second");
        Serial.println();
        DrawMenu();
        break;
      case 'C' | 'c':
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
        StartTime = micros();
        LogData();
        EndTime = micros();
        Serial.print(REC_TO_WRITE);
        Serial.print(" records written in: ");
        Serial.print(EndTime - StartTime);
        Serial.println(" uS");
        Serial.print("Performance is: ");
        Serial.print(((REC_TO_WRITE * 1000 * 1000 * SSD.getRecordLength()) / (EndTime - StartTime)));
        Serial.println(" bytes/second");
        Serial.println();
        DrawMenu();
        break;
      case 'D' | 'd':
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
    Serial.print("Field: ");
    Serial.print(i);
    Serial.print(", length (bytes): ");
    Serial.println(SSD.getFieldLength(i));
  }

  Serial.print("Record length: ");
  Serial.println(SSD.getRecordLength());
  Serial.print("Field count: ");
  Serial.println(SSD.getFieldCount());
  Serial.print("Total Records ");
  Serial.println(SSD.getLastRecord());
  Serial.print("Last RecordSetID: ");
  SSD.getLastRecord();
  Serial.println(SSD.getField(recordsetID, rID));
  Serial.println();
  Serial.println();
}

// basic data logging loop
void LogData() {

  for (i = 0; i < REC_TO_WRITE; i++) {

    // increment data point
    Point++;

    sprintf(RecordName, "Record %d", Point);

    // get some readings and process
    b0Bits = analogRead(SENSOR1_PIN);
    b1Bits = analogRead(SENSOR2_PIN);
    b2Bits = analogRead(SENSOR3_PIN);
    b3Bits = analogRead(SENSOR4_PIN);
    D2State = (byte) digitalRead(SENSOR7_PIN);

    //A0Volts = (b0Bits * 3.3) / 1024.0;
    A0Volts = (b0Bits * 3.3) / 1024.0;
    A1Volts = (b1Bits * 3.3) / 1024.0;
    A2Volts = (b2Bits * 3.3) / 1024.0;
    A3Volts = (b3Bits * 3.3) / 1024.0;
   

    SSD.addRecord();
    // save.. remember the field data are pointers that lib has direct access to

    SSD.saveRecord();
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

  uint32_t cr = SSD.getCurrentRecord();

  Serial.print("Printing records: ");
  Serial.println(SSD.getLastRecord());

  // print field headers
  Serial.print("recordsetID");
  Serial.print(", ");
  Serial.print("RecordName");
  Serial.print(", ");
  Serial.print("Point");
  Serial.print(", ");
  Serial.print("A0Volts");
  Serial.print(", ");
  Serial.print("A1Volts");
  Serial.print(", ");
  Serial.print("A2Volts");
  Serial.print(", ");
  Serial.print("A3Volts");
  Serial.print(", ");
  Serial.println("D2State");

  // print records
  // records are 1 based hence must start at 1

  for (i = 1; i <= SSD.getLastRecord(); i++) {
    SSD.gotoRecord(i);
    Serial.print(SSD.getField(recordsetID, rID));
    Serial.print(", ");
     Serial.print(SSD.getCharField(rRecordName));
    Serial.print(", ");
    Serial.print(SSD.getField(Point, rPoint));
    Serial.print(", ");
    Serial.print(SSD.getField(A0Volts, rA0Volts), 4);
    Serial.print(", ");
    Serial.print(SSD.getField(A1Volts, rA1Volts), 4);
    Serial.print(", ");
    Serial.print(SSD.getField(A2Volts, rA2Volts));
    Serial.print(", ");
    Serial.print(SSD.getField(A3Volts, rA3Volts));
    Serial.print(", ");
    Serial.println(SSD.getField(D2State, rD2State));
  }

  Serial.println("Record printing complete.");

  // restore current record
  SSD.gotoRecord(cr);

}

// dump data to and SD card
// optional, get the SD card example working first
// to debug wiring, pins etc.

void DownloadData() {

  uint32_t cr = SSD.getCurrentRecord();
  bool SDOK = false;

  if (sd.begin(10, SD_SCK_MHZ(20))) {
    Serial.println("sd card started");
  } else {
    Serial.println("no sd card");
    return;
  }

  // something you can do is create a new file for each recordset, assuming you are implementing
  // a record set approach
  // here, we'll just dump everything to the same file
  SDOK = file.open("datafile.csv", O_RDWR | O_CREAT);

  if (SDOK) {

    SSD.gotoRecord(1);

    // print field headers
    file.print("recordsetID");
    file.print(", ");
    file.print("Point");
    file.print(", ");
    file.print("A0Volts");
    file.print(", ");
    file.print("A1Volts");
    file.print(", ");
    file.print("A2Volts");
    file.print(", ");
    file.print("A3Volts");
    file.print(", ");
    file.println("D2State");

    for (i = 1; i <= SSD.getLastRecord(); i++) {
      SSD.gotoRecord(i);
      file.print(SSD.getField(recordsetID, rID));
      file.print(", ");
      file.print(SSD.getField(Point, rPoint));
      file.print(", ");
      file.print(SSD.getField(A0Volts, rA0Volts), 4);
      file.print(", ");
      file.print(SSD.getField(A1Volts, rA1Volts), 4);
      file.print(", ");
      file.print(SSD.getField(A2Volts, rA2Volts));
      file.print(", ");
      file.print(SSD.getField(A3Volts, rA3Volts));
      file.print(", ");
      file.println(SSD.getField(D2State, rD2State));
    }

    file.println("Record printing complete.");
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
