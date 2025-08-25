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

	digitalWrite(CSPin, HIGH);
	delay(20);	
	
	initStatus = readChipJEDEC();

	return initStatus;
}

 bool TeensyDB::readChipJEDEC(){
	 
	uint8_t byteID[3];

	SPI.beginTransaction(SPISettings(SPEED_WRITE, MSBFIRST, SPI_MODE0));
	digitalWrite(CSPin, LOW);
	delay(10);
	
	SPI.transfer(JEDEC);

	byteID[0] = SPI.transfer(0x00);
	byteID[1] = SPI.transfer(0x00);
	byteID[2] = SPI.transfer(0x00);

	digitalWrite(CSPin, HIGH);
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
	digitalWrite(CSPin, LOW);
	delay(10);
	
	SPI.transfer(UNIQUEID);

	SPI.transfer(0x00);
	SPI.transfer(0x00);
	SPI.transfer(0x00);
	SPI.transfer(0x00);
	
	SPI.transfer(ByteID, 8);
	
	digitalWrite(CSPin, HIGH);
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
	RecType = readByte();
	
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
	
	RecType = readByte();

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
		RecType = readByte();
		
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
		RecType = readByte();
		Address = (MiddleRecord + 1)  * RecordLength;
		NextRecType = readByte();
	
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
			readvalue = readByte();
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
	
	digitalWrite(CSPin, LOW);
	SPI.transfer(WRITEENABLE);
	digitalWrite(CSPin, HIGH);
	waitForChip(50);


	digitalWrite(CSPin, LOW);
	SPI.transfer(CHIPERASE);
	digitalWrite(CSPin, HIGH);
	waitForChip(600000);
	
	SPI.endTransaction();
	
	
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
	digitalWrite(CSPin, LOW);
	SPI.transfer(WRITEENABLE);
	digitalWrite(CSPin, HIGH);

	waitForChip(60000);

	digitalWrite(CSPin, LOW);
	SPI.transfer(CmdBytes, 4);
	digitalWrite(CSPin, HIGH);
	waitForChip(60000);
	SPI.endTransaction();
	
}

void TeensyDB::eraseSmallBlock(uint32_t BlockNumber){

	Address = BlockNumber * SMALL_BLOCK_SIZE;
	
	buildCommandBytes(CmdBytes, SMALLBLOCKERASE, Address);
		
	SPI.beginTransaction(SPISettings(SPEED_WRITE, MSBFIRST, SPI_MODE0));
	digitalWrite(CSPin, LOW);
	SPI.transfer(WRITEENABLE);
	digitalWrite(CSPin, HIGH);

	waitForChip(60000);

	digitalWrite(CSPin, LOW);
	SPI.transfer(CmdBytes, 4);
	digitalWrite(CSPin, HIGH);
	waitForChip(60000);
	SPI.endTransaction();
	
}
void TeensyDB::eraseLargeBlock(uint32_t BlockNumber){

	Address = BlockNumber * LARGE_BLOCK_SIZE;
	
	buildCommandBytes(CmdBytes, LARGEBLOCKERASE, Address);
		
	SPI.beginTransaction(SPISettings(SPEED_WRITE, MSBFIRST, SPI_MODE0));
	digitalWrite(CSPin, LOW);
	SPI.transfer(WRITEENABLE);
	digitalWrite(CSPin, HIGH);

	waitForChip(60000);

	digitalWrite(CSPin, LOW);
	SPI.transfer(CmdBytes, 4);
	digitalWrite(CSPin, HIGH);
	waitForChip(60000);
	SPI.endTransaction();
	
}


// get data
	
uint8_t TeensyDB::getField(uint8_t Data, uint8_t Field){

	Address = (CurrentRecord * RecordLength) + FieldStart[Field];
	aBytes[0] = readByte();
	
	return (uint8_t) aBytes[0];

}

int TeensyDB::getField(int Data, uint8_t Field){

	Address = (CurrentRecord * RecordLength) + FieldStart[Field];
	aBytes[0] = readByte();
	aBytes[1] = readByte();
	aBytes[2] = readByte();
	aBytes[3] = readByte();
	
	return (int) ( (aBytes[0] << 24) | (aBytes[1] << 16) | (aBytes[2] << 8) | (aBytes[3]));

}

int16_t TeensyDB::getField(int16_t Data, uint8_t Field){

	Address = (CurrentRecord * RecordLength) + FieldStart[Field];
	aBytes[0] = readByte();
	aBytes[1] = readByte();

	return (int16_t) (aBytes[0] << 8) | (aBytes[1]);

}

uint16_t TeensyDB::getField(uint16_t Data, uint8_t Field){

	Address = (CurrentRecord * RecordLength) + FieldStart[Field];
	aBytes[0] = readByte();
	aBytes[1] = readByte();

	return (uint16_t) (aBytes[0] << 8) | (aBytes[1]);

}

int32_t TeensyDB::getField(int32_t Data, uint8_t Field){

	Address = (CurrentRecord * RecordLength) + FieldStart[Field];
	aBytes[0] = readByte();
	aBytes[1] = readByte();
	aBytes[2] = readByte();
	aBytes[3] = readByte();

	return (int32_t) ( (aBytes[0] << 24) | (aBytes[1] << 16) | (aBytes[2] << 8) | (aBytes[3]));

}

uint32_t TeensyDB::getField(uint32_t Data, uint8_t Field){

	Address = (CurrentRecord * RecordLength) + FieldStart[Field];
	aBytes[0] = readByte();
	aBytes[1] = readByte();
	aBytes[2] = readByte();
	aBytes[3] = readByte();

	return (uint32_t) ( (aBytes[0] << 24) | (aBytes[1] << 16) | (aBytes[2] << 8) | (aBytes[3]));

}

float TeensyDB::getField(float Data, uint8_t Field){

	float f;

	Address = (CurrentRecord * RecordLength) + FieldStart[Field];
	
	aBytes[0] = readByte();
	aBytes[1] = readByte();
	aBytes[2] = readByte();
	aBytes[3] = readByte();	

	memcpy(&f, &aBytes, sizeof(f));
	return f;
	
}

double TeensyDB::getField(double Data, uint8_t Field){
    double d;
	Address = (CurrentRecord * RecordLength) + FieldStart[Field];
	aBytes[0] = readByte();
	aBytes[1] = readByte();
	aBytes[2] = readByte();
	aBytes[3] = readByte();	
	aBytes[4] = readByte();
	aBytes[5] = readByte();
	aBytes[6] = readByte();
	aBytes[7] = readByte();
	
	memcpy(&d, &aBytes, sizeof(d));
	return d;
}

char  *TeensyDB::getCharField(uint8_t Field){
	
	int8_t bytes[FieldLength[Field]+1];
	
	Address = (CurrentRecord * RecordLength) + FieldStart[Field];
	for (i = 0; i < FieldLength[Field];i++){
		bytes[i] = readByte();
		if (bytes[i] == '\0'){
			break;
		}		
	}
	
	memset(stng,0,FieldLength[Field]+1);
	
	memcpy(stng, bytes, FieldLength[Field]);
	//stng[sizeof(bytes)] = '\0';
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

bool TeensyDB::saveRecord() {
	
	for (i= 0; i < FieldCount; i++){		

		if (DataType[i] == DT_U8){		
			RECORD[FieldStart[i]] = *u8data[i];
		}
		else if (DataType[i] == DT_INT){			
			RECORD[FieldStart[i]] = (uint8_t) (*intdata[i] >> 24);
			RECORD[FieldStart[i]+1] = (uint8_t) (*intdata[i] >> 16);
			RECORD[FieldStart[i]+2] = (uint8_t) (*intdata[i] >> 8);
			RECORD[FieldStart[i]+3] = (uint8_t) (*intdata[i]);			
		}
		else if (DataType[i] == DT_I16){			
			RECORD[FieldStart[i]] = (uint8_t) (*i16data[i] >> 8);
			RECORD[FieldStart[i]+1] = (uint8_t) (*i16data[i]);
		}
		else if (DataType[i] == DT_U16){			
			RECORD[FieldStart[i]] = (uint8_t)(*u16data[i]  >> 8 );
			RECORD[FieldStart[i]+1] = (uint8_t) *u16data[i];
		}
		else if (DataType[i] == DT_I32){			
			RECORD[FieldStart[i]] = (uint8_t) (*i32data[i] >> 24);
			RECORD[FieldStart[i]+1] = (uint8_t) (*i32data[i] >> 16);
			RECORD[FieldStart[i]+2] = (uint8_t) (*i32data[i] >> 8);
			RECORD[FieldStart[i]+3] = (uint8_t) (*i32data[i]);
		}
		else if (DataType[i] == DT_U32){			
			RECORD[FieldStart[i]] = (uint8_t) (*u32data[i] >> 24);
			RECORD[FieldStart[i]+1] = (uint8_t) (*u32data[i] >> 16);
			RECORD[FieldStart[i]+2] = (uint8_t) (*u32data[i] >> 8);
			RECORD[FieldStart[i]+3] = (uint8_t) (*u32data[i]);
		}
		else if (DataType[i] == DT_FLOAT){	
			memcpy(aBytes, (void *)fdata[i], FieldLength[i]);			
			for (q = 0; q < FieldLength[i]; q++){
				RECORD[FieldStart[i]+q] = aBytes[q];
			}			
		}	
		else if (DataType[i] == DT_DOUBLE){				
			memcpy(aBytes, (void*)ddata[i], FieldLength[i]);
			for (q = 0; q < FieldLength[i]; q++){
				RECORD[FieldStart[i]+q] = aBytes[q];
			}	
		}
		else if (DataType[i] == DT_CHAR){				
			strcpy(buf,cdata[i]);			
			for (q = 0; q < strlen(buf); q++){
				RECORD[FieldStart[i]+q] = buf[q];
			}
			RECORD[FieldStart[i]+q] = '\0';
		}	
	}
	/*
	for (q = 0; q < RecordLength; q++){
		Serial.print(RECORD[q]);
		Serial.print("-");
	}
	Serial.println();
	*/
	writeRecord();
		
	return true;
	
}

uint8_t TeensyDB::readByte() {
	

	buildCommandBytes(CmdBytes, READ, Address);
	SPI.beginTransaction(SPISettings(SPEED_READ, MSBFIRST, SPI_MODE0));

		
	digitalWrite(CSPin, LOW);
	SPI.transfer(CmdBytes, 4);
	readvalue = SPI.transfer(0x00);
	digitalWrite(CSPin, HIGH);
	
	SPI.endTransaction();  

	waitForChip(50);

	// since we are reading byte by byte we need to advance address
	// reading byte arrays is unreliable
	Address = Address + 1;
	
	return readvalue;
  
}

void TeensyDB::waitForChip(uint32_t Wait) {
	
	timeout = millis();
	uint8_t Status = 0x01;
	
	while (Status & STAT_WIP){	
		
		digitalWrite(CSPin, LOW);
		//delayMicroseconds(5);
		SPI.transfer(CMD_READ_STATUS_REG);
		Status = SPI.transfer(0x00);
		digitalWrite(CSPin, HIGH);
		//delayMicroseconds(5);
		if ((millis() - timeout) > Wait) return; // timeout
	}
	

}

void TeensyDB::buildCommandBytes(uint8_t *buf, uint8_t cmd, uint32_t addr) {
	buf[0] = cmd;
	buf[1] = addr >> 16;
	buf[2] = addr >> 8;
	buf[3] = addr;

}

void TeensyDB::writeRecord() {
		
 	Address = CurrentRecord * RecordLength;

	// we start writing at 0 byte of current record
	// then we write in sequence RecordLength bytes
	
	uint8_t i = 0, LastByte = RecordLength;
	bool PageSpan = false;
	pageOffset = Address % PAGE_SIZE;
	
	if ((pageOffset + RecordLength) > PAGE_SIZE){
		PageSpan = true;		
		LastByte = PAGE_SIZE - pageOffset;
	}

	SPI.beginTransaction(SPISettings(SPEED_WRITE, MSBFIRST, SPI_MODE0));
	
	digitalWrite(CSPin, LOW);
	SPI.transfer(WRITEENABLE);
	digitalWrite(CSPin, HIGH); 

	waitForChip(50);

	digitalWrite(CSPin, LOW);	
	SPI.transfer(WRITE);	
	SPI.transfer((uint8_t) ((Address >> 16) & 0xFF));
	SPI.transfer((uint8_t) ((Address >> 8) & 0xFF));
	SPI.transfer((uint8_t) (Address & 0xFF));	
	
	for (i = 0; i < LastByte; i++){
		SPI.transfer(RECORD[i]);	
		Address++;		
	}
	 
	digitalWrite(CSPin, HIGH); 
		
	waitForChip(50);
	
	SPI.endTransaction();	

	if (!PageSpan){
		Address = Address + RecordLength;
		return;
	}
	SPI.beginTransaction(SPISettings(SPEED_WRITE, MSBFIRST, SPI_MODE0));
	digitalWrite(CSPin, LOW);
	SPI.transfer(WRITEENABLE);
	digitalWrite(CSPin, HIGH); 

	waitForChip(50);

	digitalWrite(CSPin, LOW);	
	SPI.transfer(WRITE);	
	SPI.transfer((uint8_t) ((Address >> 16) & 0xFF));
	SPI.transfer((uint8_t) ((Address >> 8) & 0xFF));
	SPI.transfer((uint8_t) (Address & 0xFF));	
	
	for (i = LastByte; i < RecordLength; i++){
		SPI.transfer(RECORD[i]);
		Address++;		
	}
	 
	digitalWrite(CSPin, HIGH); 
	waitForChip(50);
	SPI.endTransaction();	

}

//////////////////////////////////////////////////////////
