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

*/

#include "SPI.h"
#include "TeensyDB.h"

TeensyDB::TeensyDB(int CS_PIN) {
	
  CSPin = CS_PIN;
  
}

bool TeensyDB::init() {
	
	FieldCount = 0;
	CurrentRecord = 0;
	MaxRecords = 0;
	
	ReadComplete = false;
	
	SPI.begin();
	
	// im not a fan of delays, but some chips pin recovery is not as fast as expcted
	delay(20);	
	pinMode(CSPin, OUTPUT);

	digitalWriteFast(CSPin, HIGH);
	delay(20);	
	
	initStatus = readChipJEDEC();

	return initStatus;
}

 bool TeensyDB::readChipJEDEC(){
	 
	uint8_t byteID[3];

	SPI.beginTransaction(SPISettings(SPEED_WRITE, MSBFIRST, SPI_MODE0));
	digitalWriteFast(CSPin, LOW);
	delay(10);
	
	SPI.transfer(JEDEC);

	byteID[0] = SPI.transfer(DUMMY);
	byteID[1] = SPI.transfer(DUMMY);
	byteID[2] = SPI.transfer(DUMMY);

	digitalWriteFast(CSPin, HIGH);
	SPI.endTransaction();
	
	if ((byteID[0] == 0) || (byteID[0] == NULL_RECORD)) {
		strcpy(ChipJEDEC,"INVALID CHIP");
		return false;
	}
	else if ((byteID[1] == 0) || (byteID[1] == NULL_RECORD)) {
		strcpy(ChipJEDEC,"INVALID CHIP");
		return false;
	}
	else if ((byteID[2] == 0) || (byteID[2] == NULL_RECORD)) {
		strcpy(ChipJEDEC,"INVALID CHIP");
		return false;
	}
	else {
		sprintf(ChipJEDEC,"%02x:%02x:%02x",byteID[0],byteID[1],byteID[2]);
	}
	return true;
	
 }
 
 void TeensyDB::getUniqueID(uint8_t *ByteID){
	 
	SPI.beginTransaction(SPISettings(SPEED_WRITE, MSBFIRST, SPI_MODE0));
	digitalWriteFast(CSPin, LOW);
	delay(10);
	
	SPI.transfer(UNIQUEID);

	SPI.transfer(DUMMY);
	SPI.transfer(DUMMY);
	SPI.transfer(DUMMY);
	SPI.transfer(DUMMY);
	
	SPI.transfer(ByteID, 8);
	
	digitalWriteFast(CSPin, HIGH);
	SPI.endTransaction();
	
 }
 
 
 
 char *TeensyDB::getChipJEDEC(){
	
	return ChipJEDEC;
	
}

/*

Since flash chips tend to not allow writing over existing data. This library writes in sequential order on the chip
and will not skip address locations. Any concept of a FAT table does not exist, this library simply writes in sequential order.
Users can mimic writing to recordset1, then recordset2 then appending to recordset1. This can be done by writing a recordset value
and later exporting all data with a certian recordset. Users will have to manage their list of recordsets--remamber this library
has no FAT table for this task. This makes writing fast as free space need not be found as with a typical FAT table
The only mandatory requirement is before writing, we need to find where the end is.

If users get creative and erase sectors but omit erasing a sector with data, there can be gaps in writable addresses
this is bad, as the lib will not be able to find the real first writable address

MANDATORY: before any chip writing is done we need to find where we can start writing
commented out is a record crawling technique, that is slow, code uses bisectional search that can find the first
writabel address in 23 iterations or less

the case we searching for is record 4, as it's the first address in a writabel state

record data
1       123.21
2		1248.1
3		35.25
4		255
5		255
...		255

Also note that this database driver is 1 based meaning the first record is 1 and NOT 0.


*/


uint32_t TeensyDB::findFirstWritableRecord(){
	
	
	uint8_t RecType = 0;
	uint32_t StartRecord = 0;
	uint32_t MiddleRecord = 0;
	uint32_t EndRecord = 0;
	bool Found = false;
	uint8_t NextRecType = 0;
	uint32_t Iteration = 0;
	uint32_t MaxIteration = 0;


	// we must have some defined fields in order to compute record length
	if (RecordLength == 0) {
		// no fields
		NewCard = false;
		CurrentRecord = 0;
		LastRecord = 0;
		ReadComplete = false;
		return NO_FIELDS;
	}

	// find the maximum iterations to prevent runaway in cases where the chip was not properly erased
	// or only portions were erased--leaving gaps in data
	// such case will not let us find the first writabel record
	// to elimitate round off errors, add 1 more iteration
	MaxIteration = (log(CARD_SIZE) / log(2)) + 1;

	// get maximum possible records
	// can't more records that memory
	MaxRecords = CARD_SIZE / RecordLength;
	// we back out one record to accomodate a possible partial record
	// we back out one more since were' not starting at 0;
	MaxRecords = MaxRecords - 2;
	
	// test the first record
	gotoRecord(1);
	RecType = readData();
	
	if (RecType == NULL_RECORD){
		// no DATA
		NewCard = true;
		CurrentRecord = 0;
		LastRecord = 0;
		ReadComplete = true;
		return CHIP_NEW;
	}
	
	// test the last record
	gotoRecord(MaxRecords);
	
	RecType = readData();

	if (RecType != NULL_RECORD){
		// card full
		NewCard = false;
		CurrentRecord = MaxRecords;
		LastRecord = MaxRecords;
		ReadComplete = true;
		return CHIP_FULL;
	}
	/*
	// record crawling scheme, slow
	for (i = 1; i < MaxRecords; i++){
		gotoRecord(i);
		RecType = readData();
		
		if (RecType == NULL_RECORD){
			NewCard = false;
			LastRecord = i;
			ReadComplete = true;
			return LastRecord;
		}
		
	}
	*/
	
	// much faster alternative to finding the first writable record
	// begin the bisectional seek to get the last valid record 
	// (where record start address is not 0xFFFF and next record IS 0xFFFF)
	StartRecord = 1;
	EndRecord = MaxRecords;
		
	// OK last record is somewhere in between...
	while(!Found) {
		
		Iteration++;
		
		MiddleRecord = (EndRecord + StartRecord) / 2;
		Address = MiddleRecord * RecordLength;
		RecType = readData();
		Address = (MiddleRecord + 1)  * RecordLength;
		NextRecType = readData();
	
		if ((RecType == NULL_RECORD) && (NextRecType == NULL_RECORD)){
			// first writabel record must be before middle record
			EndRecord = MiddleRecord;
		}
		if ((RecType != NULL_RECORD) && (NextRecType != NULL_RECORD)){
			// first writabel record must be after middle record
			StartRecord = MiddleRecord;
		}
		if ((RecType != NULL_RECORD) && (NextRecType == NULL_RECORD)){
			// we found first writabel record
			LastRecord = MiddleRecord;
			Found = true;
		}
		if ((RecType == NULL_RECORD) && (NextRecType != NULL_RECORD)){
			// properly erased chip, this case should never be encountered
			// this means record 1 is NULL and next record has data
			// consider the chip full
			NewCard = false;
			CurrentRecord = MaxRecords;
			LastRecord = MaxRecords;
			ReadComplete = true;
			getAddress();
			return LastRecord;
		}
		
		if (Iteration > MaxIteration) {
			// this should never happen, but if the card was not fully erased where
			// there are gaps in used space, we may never find the first writabel record
			NewCard = true;
			MiddleRecord = 0;
			CurrentRecord = 0;
			LastRecord = 0;
			ReadComplete = true;
			getAddress();
			return CHIP_FORCE_RESTART;
		}
			
	}
	
	
	NewCard = false;
	LastRecord = MiddleRecord;
	CurrentRecord = MiddleRecord;
	getAddress();
	ReadComplete = true;
	
	return LastRecord;
	
}

void TeensyDB::findMaxRecords(){
	// get maximum possible records
	// we back out one record to accomodate a possible partial record
	// we back out one more since were' not starting at 0 but record 1;
	MaxRecords = (CARD_SIZE / RecordLength) - 2;
}

// data field addField methods
uint8_t TeensyDB::addField(uint8_t *Data) {
		
	if (FieldCount > MAX_FIELDS){
		return 0;
	}
	
	FieldCount++;
	
	DataType[FieldCount] = DT_U8;
	FieldStart[FieldCount] = RecordLength;
	FieldLength[FieldCount] =  sizeof(*Data);
	RecordLength = RecordLength + sizeof(*Data);
	u8data[FieldCount] = Data;
	findMaxRecords();
	return FieldCount;
}

uint8_t TeensyDB::addField(int *Data) {
	if (FieldCount > MAX_FIELDS){
		return 0;
	}
	FieldCount++;
	DataType[FieldCount] = DT_INT;
	FieldStart[FieldCount] = RecordLength;
	FieldLength[FieldCount] =  sizeof(*Data);
	RecordLength = RecordLength + sizeof(*Data);
	intdata[FieldCount] = Data;
	findMaxRecords();
	return FieldCount;
}

uint8_t TeensyDB::addField(int16_t *Data) {
	if (FieldCount > MAX_FIELDS){
		return 0;
	}
	
	FieldCount++;
	
	DataType[FieldCount] = DT_I16;
	FieldStart[FieldCount] = RecordLength;
	FieldLength[FieldCount] =  sizeof(*Data);
	RecordLength = RecordLength + sizeof(*Data);
	i16data[FieldCount] = Data;
	findMaxRecords();
	return FieldCount;
}

uint8_t TeensyDB::addField(uint16_t *Data) {
	if (FieldCount > MAX_FIELDS){
		return 0;
	}
	
	FieldCount++;
	
	DataType[FieldCount] = DT_U16;
	FieldStart[FieldCount] = RecordLength;
	FieldLength[FieldCount] =  sizeof(*Data);
	RecordLength = RecordLength + sizeof(*Data);
	u16data[FieldCount] = Data;
	findMaxRecords();	
	return FieldCount;
}

uint8_t TeensyDB::addField(int32_t *Data) {
	if (FieldCount > MAX_FIELDS){
		return 0;
	}
	
	FieldCount++;
	DataType[FieldCount] = DT_I32;
	FieldStart[FieldCount] = RecordLength;
	FieldLength[FieldCount] =  sizeof(*Data);
	RecordLength = RecordLength + sizeof(*Data);
	i32data[FieldCount] = Data;
	findMaxRecords();	
	return FieldCount;
}

uint8_t TeensyDB::addField(uint32_t *Data) {
	if (FieldCount > MAX_FIELDS){
		return 0;
	}
	FieldCount++;
	DataType[FieldCount] = DT_U32;
	FieldStart[FieldCount] = RecordLength;
	FieldLength[FieldCount] =  sizeof(*Data);
	RecordLength = RecordLength + sizeof(*Data);
	u32data[FieldCount] = Data;
	findMaxRecords();	
	return FieldCount;
}
uint8_t TeensyDB::addField(float *Data) {
	if (FieldCount > MAX_FIELDS){
		return 0;
	}
	
	FieldCount++;
	
	DataType[FieldCount] = DT_FLOAT;
	FieldStart[FieldCount] = RecordLength;
	FieldLength[FieldCount] =  sizeof(*Data);
	RecordLength = RecordLength + sizeof(*Data);
	fdata[FieldCount] = Data;
	findMaxRecords();	
	return FieldCount;
}

uint8_t TeensyDB::addField(double *Data) {
	if (FieldCount > MAX_FIELDS){
		return 0;
	}
	
	FieldCount++;
	
	DataType[FieldCount] = DT_DOUBLE;
	FieldStart[FieldCount] = RecordLength;
	FieldLength[FieldCount] =  sizeof(*Data);
	RecordLength = RecordLength + sizeof(*Data);
	ddata[FieldCount] = Data;
	findMaxRecords();	
	return FieldCount;
}

uint8_t TeensyDB::addField(char *Data, uint8_t len) {
	if (FieldCount > MAX_FIELDS){
		return 0;
	}
	
	FieldCount++;
	
	DataType[FieldCount] = DT_CHAR;
	FieldStart[FieldCount] = RecordLength;
	FieldLength[FieldCount] =  len;
	RecordLength = RecordLength + len;
	cdata[FieldCount] = Data;
	findMaxRecords();	
	return FieldCount;
}

void TeensyDB::listFields() {

	for (i = 1; i <= FieldCount; i++){
		Serial.print("Field: ");
		Serial.print(i);
		Serial.print(", start: ");
		Serial.print(FieldStart[i]);
		Serial.print(", Length: ");
		Serial.println(FieldLength[i]);
			
	}
	
	Serial.print("Record length: ");Serial.println(RecordLength);
	Serial.print("Field count: ");Serial.println(FieldCount);
	
}


uint32_t TeensyDB::gotoLastRecord(){
	
	CurrentRecord = LastRecord;
	getAddress();
	return CurrentRecord;
		
}

uint8_t TeensyDB::getFieldCount(){
	return FieldCount;
}

uint8_t TeensyDB::getFieldLength(uint8_t Index){
	return FieldLength[Index];
}

uint8_t TeensyDB::getFieldStart(uint8_t Index){
	return FieldStart[Index];
}

uint16_t TeensyDB::getRecordLength(){
	return RecordLength;
}

uint32_t TeensyDB::getUsedSpace(){

	return LastRecord * RecordLength;
}

uint32_t TeensyDB::getTotalSpace(){
	return CARD_SIZE;
}

void TeensyDB::setReadSpeed(bool Speed){
	readSpeed = Speed;
	if (readSpeed){
		readByteSize = 5;
	}
	else {
		readByteSize = 4;
	}
}


void TeensyDB::B2ToBytes(uint8_t *bytes, int16_t var) {
  bytes[0] = (uint8_t) (var >> 8);
  bytes[1] = (uint8_t) (var);
}

void TeensyDB::B2ToBytes(uint8_t *bytes, uint16_t var) {
  bytes[0] = (uint8_t) (var >> 8);
  bytes[1] = (uint8_t) (var);
}
void TeensyDB::B4ToBytes(uint8_t *bytes, int var) {
  bytes[0] = (uint8_t) (var >> 24);
  bytes[1] = (uint8_t) (var >> 16);
  bytes[2] = (uint8_t) (var >> 8);
  bytes[3] = (uint8_t) (var);
  
}

void TeensyDB::B4ToBytes(uint8_t *bytes, int32_t var) {
  bytes[0] = (uint8_t) (var >> 24);
  bytes[1] = (uint8_t) (var >> 16);
  bytes[2] = (uint8_t) (var >> 8);
  bytes[3] = (uint8_t) (var);
}

void TeensyDB::B4ToBytes(uint8_t *bytes, uint32_t var) {
  bytes[0] = (uint8_t) (var >> 24);
  bytes[1] = (uint8_t) (var >> 16);
  bytes[2] = (uint8_t) (var >> 8);
  bytes[3] = (uint8_t) (var);
    
}

void TeensyDB::FloatToBytes(uint8_t *bytes, float var) {
	
  memcpy(bytes, (uint8_t*) (&var), 4);
}

void TeensyDB::DoubleToBytes(uint8_t *bytes, double var) {
	
  memcpy(bytes, (uint8_t*) (&var), 8);
  
}

bool TeensyDB::addRecord(){
	
	// special case for address = 0 (new card)
	// write to address 0, otherwise advance address to next record
	// save record does not advance address it returns to it's initial address
		
	if (!ReadComplete){
		findFirstWritableRecord();
		ReadComplete = true;
	}
	
	if (CurrentRecord >= MaxRecords) {
		RecordAdded = false;
		return false;
	}
	
	// now that record is written, bump the address to the next writable
	// address
	
	CurrentRecord++;
	LastRecord++;
	RecordAdded = true;
	getAddress();
	return true;
		
}

void TeensyDB::dumpBytes() {
	
	uint32_t TempRecord = 0;
	uint32_t InvalidRecords = 0;

	TempRecord = CurrentRecord;
	
	CurrentRecord = 0;
	Serial.println("Dump bytes-------------------- "); 
	
	// keeping dumping memory until we get 0xFFFF too many times
	// this will account for any skips
	
	while (InvalidRecords < 10){
		
		Address = CurrentRecord * RecordLength;
		
		Serial.print("Address: "); Serial.print(Address);
		Serial.print(", Record: "); Serial.print(CurrentRecord); 
		Serial.print(" - ");
			
		for (i = 0; i< RecordLength; i++){
			readvalue = readData();
			if((i == 0) && (readvalue == NULL_RECORD)) {
				InvalidRecords++;
			}
			Serial.print(readvalue);
			Serial.print("-");
		}
		Serial.println("");
		
		if (CurrentRecord >= MaxRecords) {
			gotoRecord(TempRecord);
			return;
		}
		
		CurrentRecord++;

	}
	
	gotoRecord(TempRecord);

}


void TeensyDB::eraseAll(){
	
	SPI.beginTransaction(SPISettings(SPEED_WRITE, MSBFIRST, SPI_MODE0));
	
	digitalWriteFast(CSPin, LOW);
	SPI.transfer(WRITEENABLE);
	digitalWriteFast(CSPin, HIGH);
	waitForChip(60000);


	digitalWriteFast(CSPin, LOW);
	SPI.transfer(CHIPERASE);
	digitalWriteFast(CSPin, HIGH);

	
	SPI.endTransaction();
	waitForChip(60000);
	
	NewCard = true;
	ReadComplete = true;
	LastRecord = 0;
	CurrentRecord = 0;
	
	readChipJEDEC();

}
	
void TeensyDB::eraseSector(uint32_t SectorNumber){

	Address = SectorNumber * SECTOR_SIZE;
	
	buildCommandBytes(CmdBytes, SECTORERASE, Address);
		
	SPI.beginTransaction(SPISettings(SPEED_WRITE, MSBFIRST, SPI_MODE0));
	digitalWriteFast(CSPin, LOW);
	SPI.transfer(WRITEENABLE);
	digitalWriteFast(CSPin, HIGH);

	waitForChip(60000);

	digitalWriteFast(CSPin, LOW);
	SPI.transfer(CmdBytes, 4);
	digitalWriteFast(CSPin, HIGH);
	waitForChip(60000);
	SPI.endTransaction();
	
}

void TeensyDB::eraseSmallBlock(uint32_t BlockNumber){

	Address = BlockNumber * SMALL_BLOCK_SIZE;
	
	buildCommandBytes(CmdBytes, SMALLBLOCKERASE, Address);
		
	SPI.beginTransaction(SPISettings(SPEED_WRITE, MSBFIRST, SPI_MODE0));
	digitalWriteFast(CSPin, LOW);
	SPI.transfer(WRITEENABLE);
	digitalWriteFast(CSPin, HIGH);

	waitForChip(60000);

	digitalWriteFast(CSPin, LOW);
	SPI.transfer(CmdBytes, 4);
	digitalWriteFast(CSPin, HIGH);
	waitForChip(60000);
	SPI.endTransaction();
	
}
void TeensyDB::eraseLargeBlock(uint32_t BlockNumber){

	Address = BlockNumber * LARGE_BLOCK_SIZE;
	
	buildCommandBytes(CmdBytes, LARGEBLOCKERASE, Address);
		
	SPI.beginTransaction(SPISettings(SPEED_WRITE, MSBFIRST, SPI_MODE0));
	digitalWriteFast(CSPin, LOW);
	SPI.transfer(WRITEENABLE);
	digitalWriteFast(CSPin, HIGH);

	waitForChip(60000);

	digitalWriteFast(CSPin, LOW);
	SPI.transfer(CmdBytes, 4);
	digitalWriteFast(CSPin, HIGH);
	waitForChip(60000);
	SPI.endTransaction();
	
}

// get data
	
uint8_t TeensyDB::getField(uint8_t Data, uint8_t Field){

	Address = (CurrentRecord * RecordLength) + FieldStart[Field];
	a1Byte[0] = readData();
	
	return (uint8_t) a1Byte[0];

}

int TeensyDB::getField(int Data, uint8_t Field){

	Address = (CurrentRecord * RecordLength) + FieldStart[Field];
	a4Bytes[0] = readData();
	a4Bytes[1] = readData();
	a4Bytes[2] = readData();
	a4Bytes[3] = readData();
	
	return (int) ( (a4Bytes[0] << 24) | (a4Bytes[1] << 16) | (a4Bytes[2] << 8) | (a4Bytes[3]));

}

int16_t TeensyDB::getField(int16_t Data, uint8_t Field){

	Address = (CurrentRecord * RecordLength) + FieldStart[Field];
	a2Bytes[0] = readData();
	a2Bytes[1] = readData();

	return (int16_t) (a2Bytes[0] << 8) | (a2Bytes[1]);

}

uint16_t TeensyDB::getField(uint16_t Data, uint8_t Field){

	Address = (CurrentRecord * RecordLength) + FieldStart[Field];
	a2Bytes[0] = readData();
	a2Bytes[1] = readData();

	return (uint16_t) (a2Bytes[0] << 8) | (a2Bytes[1]);

}

int32_t TeensyDB::getField(int32_t Data, uint8_t Field){

	Address = (CurrentRecord * RecordLength) + FieldStart[Field];
	a4Bytes[0] = readData();
	a4Bytes[1] = readData();
	a4Bytes[2] = readData();
	a4Bytes[3] = readData();

	return (int32_t) ( (a4Bytes[0] << 24) | (a4Bytes[1] << 16) | (a4Bytes[2] << 8) | (a4Bytes[3]));

}

uint32_t TeensyDB::getField(uint32_t Data, uint8_t Field){

	Address = (CurrentRecord * RecordLength) + FieldStart[Field];
	a4Bytes[0] = readData();
	a4Bytes[1] = readData();
	a4Bytes[2] = readData();
	a4Bytes[3] = readData();

	return (uint32_t) ( (a4Bytes[0] << 24) | (a4Bytes[1] << 16) | (a4Bytes[2] << 8) | (a4Bytes[3]));

}

float TeensyDB::getField(float Data, uint8_t Field){

	float f;

	Address = (CurrentRecord * RecordLength) + FieldStart[Field];
	
	a4Bytes[0] = readData();
	a4Bytes[1] = readData();
	a4Bytes[2] = readData();
	a4Bytes[3] = readData();	

	memcpy(&f, &a4Bytes, sizeof(f));
	return f;
	
}

double TeensyDB::getField(double Data, uint8_t Field){
    double d;
	Address = (CurrentRecord * RecordLength) + FieldStart[Field];
	a8Bytes[0] = readData();
	a8Bytes[1] = readData();
	a8Bytes[2] = readData();
	a8Bytes[3] = readData();	
	a8Bytes[4] = readData();
	a8Bytes[5] = readData();
	a8Bytes[6] = readData();
	a8Bytes[7] = readData();
	
	memcpy(&d, &a8Bytes, sizeof(d));
	return d;
}

char  *TeensyDB::getCharField(uint8_t Field){
	
	int8_t bytes[MAXFIELDNAMELENGTH+2];
	
	Address = (CurrentRecord * RecordLength) + FieldStart[Field];

	for (i = 0; i < MAXFIELDNAMELENGTH;i++){
		bytes[i] = readData();
	}
	
	memset(stng,0,MAXFIELDNAMELENGTH+2);
	
	memcpy(stng, bytes, MAXFIELDNAMELENGTH+2);
	
	return stng;
}

uint32_t TeensyDB::getCurrentRecord(){
	return CurrentRecord;
	
}

uint32_t TeensyDB::getLastRecord(){
	return LastRecord;
}

void TeensyDB::setAddress(uint32_t Address){
	Address = Address;
	
}

uint32_t TeensyDB::getAddress(){
	return Address;
	
}

void TeensyDB::gotoRecord(uint32_t RecordNumber){
	
	if (RecordNumber > MaxRecords){
		CurrentRecord = MaxRecords;
	}
	else {
		CurrentRecord = RecordNumber;
	}
	
	Address = (CurrentRecord + 1)  * RecordLength;

}

uint8_t TeensyDB::readData() {
	
	if (readSpeed){
		buildCommandBytes(CmdBytes, FASTREAD, Address);
		SPI.beginTransaction(SPISettings(SPEED_READ_FAST, MSBFIRST, SPI_MODE0));
	}
	else{
		buildCommandBytes(CmdBytes, READ, Address);
		SPI.beginTransaction(SPISettings(SPEED_READ, MSBFIRST, SPI_MODE0));
	}
		
	digitalWriteFast(CSPin, LOW);
	SPI.transfer(CmdBytes, readByteSize);
	readvalue = SPI.transfer(DUMMY);
	digitalWriteFast(CSPin, HIGH);
	
	SPI.endTransaction();  

	waitForChip(40);

	// since we are reading byte by byte we need to advance address
	// reading byte arrays is unreliable
	Address = Address + 1;
	
	return readvalue;
  
}

void TeensyDB::writeData(uint8_t data) {

	buildCommandBytes(CmdBytes, WRITE, Address);
		
	SPI.beginTransaction(SPISettings(SPEED_WRITE, MSBFIRST, SPI_MODE0));
	digitalWriteFast(CSPin, LOW);
	SPI.transfer(WRITEENABLE); 
	digitalWriteFast(CSPin, HIGH); 

    waitForChip(10);

	digitalWriteFast(CSPin, LOW);
	SPI.transfer(CmdBytes, 4);  
	SPI.transfer(data); 
	digitalWriteFast(CSPin, HIGH);  

	SPI.endTransaction();

	waitForChip(10);
		
	// since we are writing byte by byte we need to advance address
	// writing byte arrays is unreliable
	Address = Address + 1;

}

void TeensyDB::waitForChip(uint32_t Wait) {
	
	timeout = millis();
	while (1) {
		SPI.beginTransaction(SPISettings(SPEED_WRITE, MSBFIRST, SPI_MODE0));
		digitalWriteFast(CSPin, LOW);
		status = SPI.transfer16(0x0500); // 0x05 = get status
		digitalWriteFast(CSPin, HIGH);
		SPI.endTransaction();
		if (!(status & 1)) break;
		if ((millis() - timeout) > Wait) return; // timeout
		
	}

	return ;

}

void TeensyDB::buildCommandBytes(uint8_t *buf, uint8_t cmd, uint32_t addr) {
	buf[0] = cmd;
	buf[1] = addr >> 16;
	buf[2] = addr >> 8;
	buf[3] = addr;
	if (readSpeed){
		buf[4] = DUMMY;
	}
}

bool TeensyDB::saveRecord() {
	
	if (!RecordAdded) {
		if (!addRecord()){
			return false;
		}
	}

	for (i= 1; i <= FieldCount; i++){
		
		Address = (CurrentRecord * RecordLength) + FieldStart[i];
		
		if (DataType[i] == DT_U8){
			writeData(*u8data[i]);
		}
		else if (DataType[i] == DT_INT){
			B4ToBytes(a4Bytes, *intdata[i]);
			writeData(a4Bytes[0]);
			writeData(a4Bytes[1]);
			writeData(a4Bytes[2]);
			writeData(a4Bytes[3]);
		}
		else if (DataType[i] == DT_I16){
			B2ToBytes(a2Bytes, *i16data[i]);
			writeData(a2Bytes[0]);
			writeData(a2Bytes[1]);
		}
		else if (DataType[i] == DT_U16){
			B2ToBytes(a2Bytes, *u16data[i]);
			writeData(a2Bytes[0]);
			writeData(a2Bytes[1]);
		}
		else if (DataType[i] == DT_I32){
			B4ToBytes(a4Bytes, *i32data[i]);
			writeData(a4Bytes[0]);
			writeData(a4Bytes[1]);
			writeData(a4Bytes[2]);
			writeData(a4Bytes[3]);
		}
		else if (DataType[i] == DT_U32){	
			B4ToBytes(a4Bytes, *u32data[i]);
			writeData(a4Bytes[0]);
			writeData(a4Bytes[1]);
			writeData(a4Bytes[2]);
			writeData(a4Bytes[3]);
		}
		else if (DataType[i] == DT_FLOAT){		
			FloatToBytes(a4Bytes, *fdata[i]);
			writeData(a4Bytes[0]);
			writeData(a4Bytes[1]);
			writeData(a4Bytes[2]);
			writeData(a4Bytes[3]);	
		}	
		else if (DataType[i] == DT_DOUBLE){		
			DoubleToBytes(a8Bytes, *ddata[i]);
			writeData(a8Bytes[0]);
			writeData(a8Bytes[1]);
			writeData(a8Bytes[2]);
			writeData(a8Bytes[3]);	
			writeData(a8Bytes[4]);
			writeData(a8Bytes[5]);
			writeData(a8Bytes[6]);
			writeData(a8Bytes[7]);	
		}		
	
		else if (DataType[i] == DT_CHAR){
		
			strcpy(buf,cdata[i]);

			for (j = 0; j < MAXDATACHARLEN; j ++){
				writeData(buf[j]);				
			}		

		}
			
	}

	RecordAdded = false;
	return true;
	
}

//////////////////////////////////////////////////////////
