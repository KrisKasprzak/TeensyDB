/*
  The MIT License (MIT)

  library writen by Kris Kasprzak
  
  Permission is hereby granted, free of charge, to any person obtaining a copy of
  this software and associated documentation files (the "Software"), to deal in
  the Software without restriction, including without limitation the rights to
  use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
  the Software, and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
  FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
  COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

  On a personal note, if you develop an application or product using this library 
  and make millions of dollars, I'm happy for you!

	rev		date			author				change
	1.0		2/2022			kasprzak			initial code
	2.0		1/2024			kasprzak			code cleanup
	2.5		8/2024			kasprzak			Optimized write time by writing full record as oppposed to byte by byte
	
	This library has been tested with
	MCU
	Teensy 3.2
	Teensy 4.0
	
	Flash Chips
	Microchip		SST25F040C
	Winbond 		25Q64JVSIQ
	
	this library is a database driver for the hardware listed above. With some changes, this library may work 
	with other flash chips such as changing chip sizes and possibly instruction codes in the #define section


*/

#ifndef TEENSYDB_H
#define TEENSYDB_H

#if ARDUINO >= 100
	 #include "Arduino.h"
	 #include "Print.h"
#else
	
#endif

#ifdef __cplusplus
	
#endif

#include <SPI.h>  

#define TEENSYDB_VERSION 2.5

#define MAX_FIELDS 20
#define TEENSYDB_MAXREXORDLENGTH 100
#define TEENSYDB_MAXDATACHARLEN 20
#define PAGE_SIZE 256

// chip size details
#define CARD_SIZE 8388608 // 32768 pages x 256
#define SECTOR_SIZE 4096
#define LARGE_BLOCK_SIZE 65536
#define SMALL_BLOCK_SIZE 32768
#define SPEED_WRITE      25000000
#define SPEED_READ       25000000

// chip instruction codes
#define NULL_RECORD 	0xFF
#define WRITEENABLE   	0x06
#define WRITE         	0x02
#define READ          	0x03
#define FASTREAD        0x0B
#define CMD_READ_STATUS_REG    0x05
#define SMALLBLOCKERASE 0x52
#define LARGEBLOCKERASE 0xD8
#define SECTORERASE   	0x20
#define CHIPERASE     	0x60
#define JEDEC         	0x9F
#define UNIQUEID     	0x4B
#define STAT_WIP 			   0x01
//
//
// end of chip specific settings
/////////////////////////////////////////////////////////

#define DT_U8 	1
#define DT_INT 	2
#define DT_I16 	3
#define DT_U16 	4
#define DT_I32 	5
#define DT_U32 	6
#define DT_FLOAT 7
#define DT_DOUBLE 8
#define DT_CHAR 9
#define DT_UINT 10

#define CHIP_NEW 0
#define CHIP_INVALID -1
#define CHIP_FULL -2
#define CHIP_FORCE_RESTART -3
#define NO_FIELDS -4

// class constructor
class  TeensyDB {
		
public:

	// just need the chip select for the flash chip, make sure pin supports fast chip select writing
	TeensyDB(int CS_PIN);
	
	// must call to initiate some settings
	bool init();
	
	// call as many as you need to establish the field list
	// fields can be in any order, but max field count is 255
	// WARNING.... if your added fields don't match the field list on the chip
	// reading and writing will be corrupted
	// if you change field list you myst erase your chip
	uint8_t addField(uint8_t *Data);	
	uint8_t addField(int *Data);
	uint8_t addField(int16_t *Data);
	uint8_t addField(uint16_t *Data);
	uint8_t addField(uint32_t *Data);
	uint8_t addField(int32_t *Data);
	uint8_t addField(float *Data);
	uint8_t addField(double *Data);
	uint8_t addField(char  *Data, uint8_t len);
	
	// method used to determine the first writable record and where the next record can begin
	// this function uses bisectional seeking to determine the end
	// the function relies on the field list being first established
	uint32_t findFirstWritableRecord();
	
	// method used to go to the last record, the address is pointing to the beginning of the last valid record
	uint32_t gotoLastRecord();
	
	// method to get the manufacture JEDEC codes
	char *getChipJEDEC();
	
	void getUniqueID(uint8_t *ByteID);
	
	// method to return the current address, mainly for debugging, and really should never be needed
	// this library is intended to be a record based (goto record x and read field y) and not have to worry about the address
	uint32_t getAddress();
	
	// method to go to a desired record, error checking if you goto 0 or past last record
	// first call to get data from a field, first goto a record the use getField to retrieve saved data
	void gotoRecord(uint32_t Record);
	
	// method to return where you are at, can be used in printing operations, where you get the current record, then
	// print other records, the you can return to that current record
	uint32_t getCurrentRecord();
	
	
	uint32_t getLastRecord();
	
	// method to just erase a portion of the chip
	// I recommend proceeding with caution with this call
	// if you erase just a portion of the data in the middle of a large data set
	// you will most likely NOT be able to successfully write new data
	// at startup a bisectional seek is used to find the last record. any breaks
	// in th data will break seeking and return a record that is NOT at the end
	// meaning you will probably write over existing data that will corrupt that data
	void eraseSector(uint32_t SectorNumber);
	
	// method to just erase a portion of the chip
	// I recommend proceeding with caution with this call
	// if you erase just a portion of the data in the middle of a large data set
	// you will most likely NOT be able to successfully write new data
	// at startup a bisectional seek is used to find the last record. any breaks
	// in th data will break seeking and return a record that is NOT at the end
	// meaning you will probably write over existing data that will corrupt that data
	void eraseSmallBlock(uint32_t BlockNumber);
	
	// method to just erase a portion of the chip
	// I recommend proceeding with caution with this call
	// if you erase just a portion of the data in the middle of a large data set
	// you will most likely NOT be able to successfully write new data
	// at startup a bisectional seek is used to find the last record. any breaks
	// in th data will break seeking and return a record that is NOT at the end
	// meaning you will probably write over existing data that will corrupt that data
	void eraseLargeBlock(uint32_t BlockNumber);
	
	// method to...you guessed it... erase the entire chip
	// by far the safest but can take 20 seconds
	void eraseAll();
	
	// method to add a new record, must be called before save record
	// it will be called automatically if saveRecord called w/o new record
	// included a manual way as it's a typical workflow
	// add rec does nothign more that move the last address by the record length
	bool addRecord();

	// method to save all set fields to their location in a new record
	// there are no provisions to save over a record. these flash chips will only write to 
	// unwritten addresses (0xFF)
	// save record looks at the datas pointers, so there are no need to pass in data
	bool saveRecord();
	
	// method to dump the field list to the serial monitor. it will dump based on the set field list
	// mainly for debugging, no known practical use	
	void listFields();
	
	// method to get the count of the set fields
	// ideal for printing the field names and field data
	// for example:
	// for i = 1 to max records
	//   for j = 1 to max fields
	//     printfield(j)
	//   next field
	// next record
	uint8_t getFieldCount();
	
	// menthod to get the field length so you can print it to some type of report
	// not really a practical need since getField will return the data
	uint8_t getFieldLength(uint8_t Index);
	
	// menthod to get the record length 
	// maybe useful for computing data set size (records * recordlength)
	uint16_t getRecordLength();

	// method to return the used space
	// simple (total records * recordlength)
	uint32_t getUsedSpace();
	
	// method to return the chips card size as defined above in a #define
	uint32_t getTotalSpace();	
			
	// overloaded functions to getField data
	// odd to pass variable in, but that is whats used
	// to determine byte size to get and convert to a specific data type
	// I'm happy to hear of a better way
	uint8_t getField(uint8_t Data, uint8_t Field);
	int getField(int Data, uint8_t Field);
	int16_t getField(int16_t Data, uint8_t Field);
	uint16_t getField(uint16_t Data, uint8_t Field);
	int32_t getField(int32_t Data, uint8_t Field);
	uint32_t getField(uint32_t Data, uint8_t Field);
	float getField(float Data, uint8_t Field);
	double getField(double Data, uint8_t Field);
	char *getCharField(uint8_t Field);

		// method to dump bytes to the serial monitor. it will dump based on the set field list
	// mainly for debugging, no known practical use
	void dumpBytes();
				
private:

	// only important items will be explained
	uint8_t CSPin;
	unsigned long bt = 0;
	bool RecordAdded = false;
	bool ReadComplete = false;
	char stng[TEENSYDB_MAXDATACHARLEN];
	uint8_t RECORD[TEENSYDB_MAXREXORDLENGTH];
	bool initStatus = false;
	uint32_t timeout = 0;
	char ChipJEDEC[15];

	uint8_t CmdBytes[5]; 
	uint8_t aBytes[8];
	bool NewCard = false;
	uint8_t readvalue;
	uint32_t TempAddress = 0;
	volatile uint32_t Address = 0;
	uint32_t MaxRecords = 0;
	uint32_t LastRecord = 0;
	uint32_t CurrentRecord = 0;
	uint32_t i = 0, j = 0;
	uint8_t q = 0;
	uint8_t FieldCount = 0;
	uint16_t status = 0;
	uint8_t RecordLength;
	int16_t pagesize;
	size_t pageOffset;
	uint8_t DataType[MAX_FIELDS];
	uint8_t FieldStart[MAX_FIELDS];
	uint8_t FieldLength[MAX_FIELDS];
	uint8_t *u8data[MAX_FIELDS];
	int *intdata[MAX_FIELDS];
	int16_t *i16data[MAX_FIELDS];
	uint16_t *u16data[MAX_FIELDS];
	int32_t *i32data[MAX_FIELDS];
	uint32_t *u32data[MAX_FIELDS];
	float *fdata[MAX_FIELDS];
	double *ddata[MAX_FIELDS];
	char *cdata[MAX_FIELDS];
	char buf[TEENSYDB_MAXDATACHARLEN];
	
	unsigned char wip_check;
	uint32_t st = 0;
	unsigned char ret = 0;
	
	void writeRecord();
	bool readChipJEDEC();

	// method to read data to the chip one byte at a time
	// ReadData and SetAddress could be made public for getting data from the chip
	// in an emergency situation
	uint8_t readByte();
	void setAddress(uint32_t Address);
	
	// method to save data on a field by field basis
	// recall this library is a record/field database
	//void saveField(uint8_t *Data, uint8_t Field);

	// method to read chips status to see if it's done
	void waitForChip(uint32_t Wait);
	
	// methods to convert data to by equivalent
	void B2ToBytes(uint8_t *bytes, int16_t var);
	void B2ToBytes(uint8_t *bytes, uint16_t var);
	void B4ToBytes(uint8_t *bytes, int var);
	void B4ToBytes(uint8_t *bytes, int32_t var);
	void B4ToBytes(uint8_t *bytes, uint32_t var);
	void FloatToBytes(uint8_t *bytes, float var);
	void DoubleToBytes(uint8_t *bytes, double var);
	
	// method to find max possible records
	// done by taking chip size and dividing by record length
	// and backing out 2 from possible partial record and the fact
	// that we are starting at 1 (first record lenght is skipped)
	void findMaxRecords();
	
	// function to build byte list of command + 24 bit address
	void buildCommandBytes(uint8_t *buf, uint8_t cmd, uint32_t addr);
	
	// menthod to get the field length so you can print it to some type of report
	// not really a practical need since getField will return the data
	uint8_t getFieldStart(uint8_t Index);
		

	
};



#endif
