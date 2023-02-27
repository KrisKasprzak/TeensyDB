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
#include "DBase.h"

void DBase::DebugData(int Line){
	
	Serial.print("Line: "); Serial.print(Line); 
	Serial.print(" Address: "); Serial.print(Address); 
	Serial.print(" Current Rec: "); Serial.print(CurrentRecord); 
	Serial.print(" Max Rec  : "); Serial.println(MaxRecords); 

}

DBase::DBase(int CS_PIN) {
	
  cspin = CS_PIN;
  
}

bool DBase::init() {
	
	int16_t Ret;
	
	FieldCount = 0;
	CurrentRecord = 0;
	MaxRecords = 0;
	
	ReadComplete = false;
	
	SPI.begin();
	delay(20);	
	pinMode(cspin, OUTPUT);

	digitalWriteFast(cspin, HIGH);
	delay(20);	

	Ret = readChipJEDEC();

	return Ret;
}

 bool DBase::readChipJEDEC(){
	 
	uint8_t byteID[3];

	////////////////////////////////////////////////////////////////////////
	SPI.beginTransaction(SPISettings(SPEED_WRITE, MSBFIRST, SPI_MODE0));
	digitalWriteFast(cspin, LOW);
	delay(10);
	
	SPI.transfer(JEDEC);

	byteID[0] = SPI.transfer(0x00);
	byteID[1] = SPI.transfer(0x00);
	byteID[2] = SPI.transfer(0x00);

	digitalWriteFast(cspin, HIGH);
	SPI.endTransaction();
	
	//Serial.println("Chip JEDEC"); 
	//Serial.print("byteID[0] "); Serial.println(byteID[0], HEX);
	//Serial.print("byteID[1] "); Serial.println(byteID[1], HEX);
	//Serial.print("byteID[2] "); Serial.println(byteID[2], HEX);

	
	if ((byteID[0] == 0) || (byteID[0] == NULL_RECORD)) {
		strcpy(ChipJEDEC,"INVALID CHIP");
		//return false;
	}
	else if ((byteID[1] == 0) || (byteID[1] == NULL_RECORD)) {
		strcpy(ChipJEDEC,"INVALID CHIP");
		//return false;
	}
	else if ((byteID[2] == 0) || (byteID[2] == NULL_RECORD)) {
		strcpy(ChipJEDEC,"INVALID CHIP");
		//return false;
	}
	else {
		sprintf(ChipJEDEC,"%02x:%02x:%02x",byteID[0],byteID[1],byteID[2]);
		//Serial.print("Chip data: ");Serial.println(ChipJEDEC);
	}
	return true;
	
 }
 
 char *DBase::getChipJEDEC(){
	
	return ChipJEDEC;
	
}

uint32_t DBase::findLastRecord(){
	
	uint32_t StartRecord = 0;
	uint32_t MiddleRecord = 0;
	uint32_t EndRecord = 0;
	bool Found = false;
	uint8_t RecType = 0;
	uint8_t NextRecType = 0;
	uint32_t Iteration = 0;


	// test first record
	Address = 1 * RecordLength;

	RecType = ReadData();

	if (RecType == NULL_RECORD){
		// no DATA
		//Serial.println("no data");
		NewCard = true;
		CurrentRecord = 0;
		LastRecord = 0;

		ReadComplete = true;
		return LastRecord;
	}
	
	// test end
	Address = MaxRecords * RecordLength;
	RecType = ReadData();

	if (RecType != NULL_RECORD){
		// card full
		//Serial.println("card full");

		NewCard = false;
		CurrentRecord = MaxRecords;
		LastRecord = MaxRecords;
		ReadComplete = true;
		return LastRecord;
	}
	
	// begin the bisectional seek to get the last valid record 
	// (where record start address is not 0xFFFF and next record IS 0xFFFF)
	StartRecord = 1;
	EndRecord = MaxRecords;
	
	// OK last record is somewhere in between...
	while(!Found) {
		
		Iteration++;
		// Serial.print("Iterations: "); Serial.println(Iteration);
		MiddleRecord = (EndRecord + StartRecord) / 2;
		Address = MiddleRecord * RecordLength;
		//Serial.print("MiddleRecord ");Serial.println(MiddleRecord );

		RecType = ReadData();
		//Serial.print("RecType ");Serial.println(RecType );
	
		Address = (MiddleRecord + 1)  * RecordLength;
		//Serial.print("MiddleRecord +1");Serial.println(MiddleRecord + 1 );
		NextRecType = ReadData();
	    //Serial.print("NextRecType ");Serial.println(NextRecType );
	
		if ((RecType == NULL_RECORD) && (NextRecType == NULL_RECORD)){
			//Serial.println("Its lower");
			EndRecord = MiddleRecord;
		}
		if ((RecType != NULL_RECORD) && (NextRecType != NULL_RECORD)){
			//Serial.println("Its higher");
			StartRecord = MiddleRecord;
		}
		if ((RecType != NULL_RECORD) && (NextRecType == NULL_RECORD)){
			// we found the end
			//Serial.print("We found the end: "); Serial.println(Address);
			//Serial.print("Iterations: "); Serial.println(Iteration);
			LastRecord = MiddleRecord;
			Found = true;
		}
		
		if (Iteration > 200) { // 23 bits is max iteration for 23 bit for this chip, 1 more for good luck
			//Serial.println("timeout issue");
			//DebugData(134);
			NewCard = true;
			CurrentRecord = 0;
			LastRecord = 0;
			ReadComplete = true;
			return CHIP_FORCE_RESTART;
		}
			
	}
	
	
	NewCard = false;
	LastRecord = MiddleRecord;
	CurrentRecord = MiddleRecord;
	
	ReadComplete = true;
		
	return LastRecord;
	
}

// data field addField methods
uint8_t DBase::addField(const char *FieldName, uint8_t *Data) {
		
	if (FieldCount > MAX_FIELDS){
		return 0;
	}
	
	FieldCount++;
	
	DataType[FieldCount] = DT_U8;
	FieldStart[FieldCount] = RecordLength;
	FieldLength[FieldCount] =  sizeof(*Data);
	RecordLength = RecordLength + sizeof(*Data);
	strcpy(fieldname[FieldCount], FieldName);
	u8data[FieldCount] = Data;
	
	
	// get maximum possible records
	// we back out one record to accomodate a possible partial record
	// we back out one more since were' not starting at 0;
	MaxRecords = (CARD_SIZE / RecordLength) - 2;
	
	return FieldCount;
}

uint8_t DBase::addField(const char *FieldName, int *Data) {
	if (FieldCount > MAX_FIELDS){
		return 0;
	}
	
	FieldCount++;
	
	DataType[FieldCount] = DT_INT;
	FieldStart[FieldCount] = RecordLength;
	FieldLength[FieldCount] =  sizeof(*Data);
	RecordLength = RecordLength + sizeof(*Data);
	strcpy(fieldname[FieldCount], FieldName);
	intdata[FieldCount] = Data;
	
	// get maximum possible records
	// we back out one record to accomodate a possible partial record
	// we back out one more since were' not starting at 0;
	MaxRecords = (CARD_SIZE / RecordLength) - 2;
	
	return FieldCount;
	}
	
uint8_t DBase::addField(const char *FieldName, int16_t *Data) {
	if (FieldCount > MAX_FIELDS){
		return 0;
	}
	
	FieldCount++;
	
	DataType[FieldCount] = DT_I16;
	FieldStart[FieldCount] = RecordLength;
	FieldLength[FieldCount] =  sizeof(*Data);
	RecordLength = RecordLength + sizeof(*Data);
	strcpy(fieldname[FieldCount], FieldName);
	i16data[FieldCount] = Data;
	// get maximum possible records
	// we back out one record to accomodate a possible partial record
	// we back out one more since were' not starting at 0;
	MaxRecords = (CARD_SIZE / RecordLength) - 2;
	return FieldCount;
	}

uint8_t DBase::addField(const char *FieldName, uint16_t *Data) {
	if (FieldCount > MAX_FIELDS){
		return 0;
	}
	
	FieldCount++;
	

	DataType[FieldCount] = DT_U16;
	FieldStart[FieldCount] = RecordLength;
	FieldLength[FieldCount] =  sizeof(*Data);
	RecordLength = RecordLength + sizeof(*Data);
	strcpy(fieldname[FieldCount], FieldName);
	u16data[FieldCount] = Data;
	// get maximum possible records
	// we back out one record to accomodate a possible partial record
	// we back out one more since were' not starting at 0;
	MaxRecords = (CARD_SIZE / RecordLength) - 2;	
	return FieldCount;
}

uint8_t DBase::addField(const char *FieldName, int32_t *Data) {
	if (FieldCount > MAX_FIELDS){
		return 0;
	}
	
	FieldCount++;
	
	DataType[FieldCount] = DT_U32;
	FieldStart[FieldCount] = RecordLength;
	FieldLength[FieldCount] =  sizeof(*Data);
	RecordLength = RecordLength + sizeof(*Data);
	strcpy(fieldname[FieldCount], FieldName);
	i32data[FieldCount] = Data;
	// get maximum possible records
	// we back out one record to accomodate a possible partial record
	// we back out one more since were' not starting at 0;
	MaxRecords = (CARD_SIZE / RecordLength) - 2;	
	return FieldCount;
}

uint8_t DBase::addField(const char *FieldName, uint32_t *Data) {
	if (FieldCount > MAX_FIELDS){
		return 0;
	}
	
	FieldCount++;
	
	DataType[FieldCount] = DT_U32;
	FieldStart[FieldCount] = RecordLength;
	FieldLength[FieldCount] =  sizeof(*Data);
	RecordLength = RecordLength + sizeof(*Data);
	strcpy(fieldname[FieldCount], FieldName);
	u32data[FieldCount] = Data;
	// get maximum possible records
	// we back out one record to accomodate a possible partial record
	// we back out one more since were' not starting at 0;
	MaxRecords = (CARD_SIZE / RecordLength) - 2;	
	return FieldCount;
}
uint8_t DBase::addField(const char *FieldName, float *Data) {
	if (FieldCount > MAX_FIELDS){
		return 0;
	}
	
	FieldCount++;
	
	DataType[FieldCount] = DT_FLOAT;
	FieldStart[FieldCount] = RecordLength;
	FieldLength[FieldCount] =  sizeof(*Data);
	RecordLength = RecordLength + sizeof(*Data);
	strcpy(fieldname[FieldCount], FieldName);
	fdata[FieldCount] = Data;
	// get maximum possible records
	// we back out one record to accomodate a possible partial record
	// we back out one more since were' not starting at 0;
	MaxRecords = (CARD_SIZE / RecordLength) - 2;	
	return FieldCount;
}

uint8_t DBase::addField(const char *FieldName, double *Data) {
	if (FieldCount > MAX_FIELDS){
		return 0;
	}
	
	FieldCount++;
	
	DataType[FieldCount] = DT_DOUBLE;
	FieldStart[FieldCount] = RecordLength;
	FieldLength[FieldCount] =  sizeof(*Data);
	RecordLength = RecordLength + sizeof(*Data);
	strcpy(fieldname[FieldCount], FieldName);
	ddata[FieldCount] = Data;
	// get maximum possible records
	// we back out one record to accomodate a possible partial record
	// we back out one more since were' not starting at 0;
	MaxRecords = (CARD_SIZE / RecordLength) - 2;	
	return FieldCount;
}

uint8_t DBase::addField(const char *FieldName, char *Data, uint8_t len) {
	if (FieldCount > MAX_FIELDS){
		return 0;
	}
	
	FieldCount++;
	
	DataType[FieldCount] = DT_CHAR;
	FieldStart[FieldCount] = RecordLength;
	FieldLength[FieldCount] =  len;
	RecordLength = RecordLength + len;
	strcpy(fieldname[FieldCount], FieldName);
	cdata[FieldCount] = Data;
	// get maximum possible records
	// we back out one record to accomodate a possible partial record
	// we back out one more since were' not starting at 0;
	MaxRecords = (CARD_SIZE / RecordLength) - 2;	
	return FieldCount;
}

void DBase::listFields() {

	for (i = 1; i <= FieldCount; i++){
		Serial.print("Number: ");
		Serial.print(i);
		Serial.print(", Name: ");
		Serial.print(fieldname[i]);
		Serial.print(", type: ");
		if( DataType[i] == DT_U8){
			Serial.print("uint8_t");
		}
		else if( DataType[i] == DT_INT){
			Serial.print("int");
		}
		else if( DataType[i] == DT_I16){
			Serial.print("int16_t");
		}
		else if( DataType[i] == DT_U16){
			Serial.print("uint16_t");
		}
		else if( DataType[i] == DT_I32){
			Serial.print("int32_t");
		}
		else if( DataType[i] == DT_U32){
			Serial.print("uint32_t");
		}
		else if( DataType[i] == DT_FLOAT){
			Serial.print("float");
		}
		else if( DataType[i] == DT_DOUBLE){
			Serial.print("double");
		}
		else if( DataType[i] == DT_CHAR){
			Serial.print("char");
		}
		
		Serial.print(", start: ");
		Serial.print(FieldStart[i]);
		Serial.print(", Length: ");
		Serial.println(FieldLength[i]);
			
	}
	
	Serial.print("Record length: ");Serial.println(RecordLength);
	Serial.print("Field count: ");Serial.println(FieldCount);
	
}


uint32_t DBase::gotoLastRecord(){
	
	CurrentRecord = LastRecord;
	return CurrentRecord;
		
}

uint8_t DBase::getFieldCount(){
	return FieldCount;
}

char * DBase::getFieldName(uint8_t Index){
	return fieldname[Index];
}

uint8_t DBase::getFieldLength(uint8_t Index){
	return FieldLength[Index];
}

uint16_t DBase::getRecordLength(){
	return RecordLength;
}

uint32_t DBase::getUsedSpace(){
	return LastRecord * RecordLength;
}

uint32_t DBase::getTotalSpace(){
	return CARD_SIZE;
}

void DBase::B2ToBytes(uint8_t *bytes, int16_t var) {
  bytes[0] = (uint8_t) (var >> 8);
  bytes[1] = (uint8_t) (var);
}

void DBase::B2ToBytes(uint8_t *bytes, uint16_t var) {
  bytes[0] = (uint8_t) (var >> 8);
  bytes[1] = (uint8_t) (var);
}
void DBase::B4ToBytes(uint8_t *bytes, int var) {
  bytes[0] = (uint8_t) (var >> 24);
  bytes[1] = (uint8_t) (var >> 16);
  bytes[2] = (uint8_t) (var >> 8);
  bytes[3] = (uint8_t) (var);
}
void DBase::B4ToBytes(uint8_t *bytes, int32_t var) {
  bytes[0] = (uint8_t) (var >> 24);
  bytes[1] = (uint8_t) (var >> 16);
  bytes[2] = (uint8_t) (var >> 8);
  bytes[3] = (uint8_t) (var);
}

void DBase::B4ToBytes(uint8_t *bytes, uint32_t var) {
  bytes[0] = (uint8_t) (var >> 24);
  bytes[1] = (uint8_t) (var >> 16);
  bytes[2] = (uint8_t) (var >> 8);
  bytes[3] = (uint8_t) (var);
}

void DBase::FloatToBytes(uint8_t *bytes, float var) {
  memcpy(bytes, (unsigned char*) (&var), 4);
}

void DBase::DoubleToBytes(uint8_t *bytes, double var) {
  memcpy(bytes, (unsigned char*) (&var), 8);
}

bool DBase::addRecord(){
	
	// special case for address = 0 (new card)
	// write to address 0, otherwise advance address to next record
	// save record does not advance address it returns to it's initial address
		
	if (!ReadComplete){
		findLastRecord();
		ReadComplete = true;
	}
	//Serial.print("533 CurrentRecord "); Serial.println(CurrentRecord);
	//Serial.print("534 MaxRecords    "); Serial.println(MaxRecords);
	
	if (CurrentRecord >= MaxRecords) {
		RecordAdded = false;
		//Serial.println("addRecord failed: CurrentRecord > MaxRecords");
		return false;
	}
	
	// now that record is written, bump the address to the next writable
	// address
	//Serial.print(" 655 CurrentRecord() ");Serial.println(CurrentRecord);
	
	CurrentRecord++;
	LastRecord++;
	RecordAdded = true;
	
	// Serial.print(" 659 CurrentRecord() ");Serial.println(CurrentRecord);
	return true;
		
}

void DBase::dumpBytes() {
	
	uint32_t TempRecord = 0;
	uint8_t data = 0;
	uint32_t InvalidRecords = 0;

	TempRecord = CurrentRecord;
	
	CurrentRecord = 0;
	Serial.println("Dump bytes-------------------- "); 
	
	// keeping dumping memory until we get 0xFFFF too many times
	// this will account for any skips (assuming don't have more than 100 skipped records)
	
	while (InvalidRecords < 5000){
		
		Address = CurrentRecord * RecordLength;
		
		Serial.print("Address: "); Serial.print(Address);
		Serial.print(", Record: "); Serial.print(CurrentRecord); 
		Serial.print(" - ");
			
		for (j = 0; j< RecordLength; j++){
			data = ReadData();
			if ((j == 0) && (data == NULL_RECORD)){
				InvalidRecords++;
			}
			Serial.print(data);
			Serial.print("-");
		}
		Serial.println("");
		
//		if (CurrentRecord >= MaxRecords) {
//			gotoRecord(TempRecord);
//			return;
//		}
		
		CurrentRecord++;

	}
	
	gotoRecord(TempRecord);

}


		
bool DBase::saveRecord() {
	
	if (!RecordAdded) {
		if (!addRecord()){
			return false;
		}
	}

	// DebugData(728);
	for (i= 1; i <= FieldCount; i++){
		
		Address = (CurrentRecord * RecordLength) + FieldStart[i];
		
		if (DataType[i] == DT_U8){
			WriteData(*u8data[i]);
		}
		else if (DataType[i] == DT_INT){			
			B4ToBytes(a4Bytes, *i16data[i]);
			WriteData(a4Bytes[0]);
			WriteData(a4Bytes[1]);
			WriteData(a4Bytes[2]);
			WriteData(a4Bytes[3]);
		}
		else if (DataType[i] == DT_I16){
			B2ToBytes(a2Bytes, *i16data[i]);
			WriteData(a2Bytes[0]);
			WriteData(a2Bytes[1]);
		}
		else if (DataType[i] == DT_U16){
			B2ToBytes(a2Bytes, *u16data[i]);
			WriteData(a2Bytes[0]);
			WriteData(a2Bytes[1]);
		}
		else if (DataType[i] == DT_I32){
			
			//Serial.print(i); Serial.print(" , "); Serial.println(*i32data[i]);
			B4ToBytes(a4Bytes, *i32data[i]);
			WriteData(a4Bytes[0]);
			WriteData(a4Bytes[1]);
			WriteData(a4Bytes[2]);
			WriteData(a4Bytes[3]);
		}
		else if (DataType[i] == DT_U32){
			//Serial.print(i); Serial.print(" , "); Serial.println(*u32data[i]);
			
			B4ToBytes(a4Bytes, *u32data[i]);
			WriteData(a4Bytes[0]);
			WriteData(a4Bytes[1]);
			WriteData(a4Bytes[2]);
			WriteData(a4Bytes[3]);
		}
		else if (DataType[i] == DT_FLOAT){		
			FloatToBytes(a4Bytes, *fdata[i]);
		
			WriteData(a4Bytes[0]);
			WriteData(a4Bytes[1]);
			WriteData(a4Bytes[2]);
			WriteData(a4Bytes[3]);	
		}	
		else if (DataType[i] == DT_DOUBLE){		
			DoubleToBytes(a8Bytes, *ddata[i]);
			WriteData(a8Bytes[0]);
			WriteData(a8Bytes[1]);
			WriteData(a8Bytes[2]);
			WriteData(a8Bytes[3]);	
			WriteData(a8Bytes[4]);
			WriteData(a8Bytes[5]);
			WriteData(a8Bytes[6]);
			WriteData(a8Bytes[7]);	
		}		
	
		else if (DataType[i] == DT_CHAR){
				
			strcpy(buf,cdata[i]);
			// Serial.print("Printing char record: ");
			for (j = 0; j < MAXDATACHARLEN; j ++){
				WriteData(buf[j]);	
				// Serial.print(buf[j]);				
			}		
			// Serial.println("");
		}
			
	}

	// DebugData(865);
	// no real checking
	// todo read recs and make sure???
	RecordAdded = false;
	return true;
	
}

void DBase::eraseAll(){
	
	
	//Serial.print("697 max records    ");	Serial.println(MaxRecords);
	//Serial.print("698 Current record ");	Serial.println(CurrentRecord);
	
	SPI.beginTransaction(SPISettings(SPEED_WRITE, MSBFIRST, SPI_MODE0));
	

	waitForWrite = 1;
	writePause(30000);
	digitalWriteFast(cspin, LOW);
	SPI.transfer(WRITEENABLE);
	digitalWriteFast(cspin, HIGH);
	writePause(30000);
	//delay(500);
	digitalWriteFast(cspin, LOW);
	SPI.transfer(CHIPERASE);
	digitalWriteFast(cspin, HIGH);
	waitForWrite = 1;
	writePause(30000);
	SPI.endTransaction();
	NewCard = true;
	ReadComplete = true;
	LastRecord = 0;
	CurrentRecord = 0;
	
	readChipJEDEC();
	// findLastRecord();


}
	
void DBase::erasePage(uint32_t PageNumber){

	Address = PageNumber * PAGE_SIZE;
	SPI.beginTransaction(SPISettings(SPEED_WRITE, MSBFIRST, SPI_MODE0));
	digitalWriteFast(cspin, LOW);
	SPI.transfer(WRITEENABLE); // write instruction
	digitalWriteFast(cspin, HIGH);
	SPI.endTransaction();
	waitForWrite = 1;
	writePause(1000);
	
	SPI.beginTransaction(SPISettings(SPEED_WRITE, MSBFIRST, SPI_MODE0));
	digitalWriteFast(cspin, LOW);
	SPI.transfer(CMD_SECTOR_ERASE); // write instruction
	SPI.transfer((Address >> 16) & 0xFF);
	SPI.transfer((Address >> 8) & 0xFF);
	SPI.transfer(Address & 0xFF);
	digitalWriteFast(cspin, HIGH);
	writePause(1000);
	SPI.endTransaction();
	waitForWrite = 1;
	
	
}


//////////////////////////////////////////////////////////////////////////////////
// get data
	
uint8_t DBase::getField(uint8_t Data, uint8_t Field){

	Address = (CurrentRecord * RecordLength) + FieldStart[Field];
	a1Byte[0] = ReadData();

	return (uint8_t) a1Byte[0];

}

int DBase::getField(int Data, uint8_t Field){

	Address = (CurrentRecord * RecordLength) + FieldStart[Field];
	a4Bytes[0] = ReadData();
	a4Bytes[1] = ReadData();
	a4Bytes[2] = ReadData();
	a4Bytes[3] = ReadData(); //6

	return (int) ( (a4Bytes[0] << 24) | (a4Bytes[1] << 16) | (a4Bytes[2] << 8) | (a4Bytes[3]));

}

int16_t DBase::getField(int16_t Data, uint8_t Field){

	Address = (CurrentRecord * RecordLength) + FieldStart[Field];
	a2Bytes[0] = ReadData();
	a2Bytes[1] = ReadData();

	return (int16_t) (a2Bytes[0] << 8) | (a2Bytes[1]);

}

uint16_t DBase::getField(uint16_t Data, uint8_t Field){

	Address = (CurrentRecord * RecordLength) + FieldStart[Field];
	a2Bytes[0] = ReadData();
	a2Bytes[1] = ReadData();

	return (uint16_t) (a2Bytes[0] << 8) | (a2Bytes[1]);

}

int32_t DBase::getField(int32_t Data, uint8_t Field){

	Address = (CurrentRecord * RecordLength) + FieldStart[Field];
	a4Bytes[0] = ReadData();
	a4Bytes[1] = ReadData();
	a4Bytes[2] = ReadData();
	a4Bytes[3] = ReadData(); //6

	return (int32_t) ( (a4Bytes[0] << 24) | (a4Bytes[1] << 16) | (a4Bytes[2] << 8) | (a4Bytes[3]));

}

uint32_t DBase::getField(uint32_t Data, uint8_t Field){

	Address = (CurrentRecord * RecordLength) + FieldStart[Field];
	a4Bytes[0] = ReadData();
	a4Bytes[1] = ReadData();
	a4Bytes[2] = ReadData();
	a4Bytes[3] = ReadData(); //6

	return (uint32_t) ( (a4Bytes[0] << 24) | (a4Bytes[1] << 16) | (a4Bytes[2] << 8) | (a4Bytes[3]));

}

float DBase::getField(float Data, uint8_t Field){

	Address = (CurrentRecord * RecordLength) + FieldStart[Field];
	a4Bytes[0] = ReadData();
	a4Bytes[1] = ReadData();
	a4Bytes[2] = ReadData();
	a4Bytes[3] = ReadData();	
	
	return *(float *)a4Bytes;
}

double DBase::getField(double Data, uint8_t Field){

	Address = (CurrentRecord * RecordLength) + FieldStart[Field];
	a4Bytes[0] = ReadData();
	a4Bytes[1] = ReadData();
	a4Bytes[2] = ReadData();
	a4Bytes[3] = ReadData();	

	return *(double *)a4Bytes;
}

char  *DBase::getCharField(uint8_t Field){
	
	Address = (CurrentRecord * RecordLength) + FieldStart[Field];

	for (i = 0; i < len;i++){
		bytes[i] = ReadData();
	}
	
	memset(stng,0,MAXDATACHARLEN);
	
	memcpy(stng, bytes, len);
	// Serial.print("1101 char  "); Serial.println(stng);
	
	return stng;
}

uint8_t DBase::getFieldDataType(uint8_t Index){
	return DataType[Index];
	
}

uint32_t DBase::getCurrentRecord(){
	return CurrentRecord;
	
}

uint32_t DBase::getLastRecord(){
	return LastRecord;
	
}

uint32_t DBase::getMaxRecords(){
	return MaxRecords;
	
}


void DBase::setAddress(uint32_t Address){
	Address = Address;
	
}

uint32_t DBase::getAddress(){
	return Address;
	
}

void DBase::gotoRecord(uint32_t RecordNumber){
	
	if (RecordNumber > MaxRecords){
		CurrentRecord = MaxRecords;
	}
	else {
		CurrentRecord = RecordNumber;
	}

}

void DBase::writePause(uint16_t Wait)
{
	uint32_t st = millis();
	
  if (waitForWrite) {
    while (readStatus() & STAT_WIP){
		if ((millis() - st) > Wait) {
			return;
		}
	}
    waitForWrite = 0;
  }
}

unsigned char DBase::readStatus(void)

{
  
  digitalWriteFast(cspin, LOW);
  SPI.transfer(CMD_READ_STATUS_REG);
  c = SPI.transfer(0x00);
  digitalWriteFast(cspin, HIGH);
  return (c);
}

uint8_t DBase::ReadData() {
	

  SPI.beginTransaction(SPISettings(SPEED_READ, MSBFIRST, SPI_MODE0));

  digitalWriteFast(cspin, LOW);

  SPI.transfer(READ); // read instruction
  // SPI.transfer(FASTREAD); // read instruction
  SPI.transfer((uint8_t) ((Address >> 16) & 0xFF));
  SPI.transfer((uint8_t) ((Address >> 8) & 0xFF));
  SPI.transfer((uint8_t) (Address & 0xFF));
  readvalue = SPI.transfer(0x00);

  digitalWriteFast(cspin, HIGH); 

  SPI.endTransaction();  
 
  waitForWrite = 1;

  writePause(10);

  // since we are reading byte by byte we need to advance address
  // reading byte arrays is unreliable--not sure why
  Address = Address + 1;
  return readvalue;
  
}

void DBase::WriteData(uint8_t data) {


	SPI.beginTransaction(SPISettings(SPEED_WRITE, MSBFIRST, SPI_MODE0));
	digitalWriteFast(cspin, LOW);
	SPI.transfer(WRITEENABLE); // write instruction
	digitalWriteFast(cspin, HIGH); 
	SPI.endTransaction();
	
	waitForWrite = 1;
	writePause(10);
   
	SPI.beginTransaction(SPISettings(SPEED_WRITE, MSBFIRST, SPI_MODE0));
	digitalWriteFast(cspin, LOW);
	SPI.transfer(WRITE); // write instruction
	SPI.transfer((uint8_t) ((Address >> 16) & 0xFF));
	SPI.transfer((uint8_t) ((Address >> 8) & 0xFF));
	SPI.transfer((uint8_t) (Address & 0xFF));
	SPI.transfer(data);
	//SPI.transfer(255); // data sheet says to write this...   
	digitalWriteFast(cspin, HIGH);  
	SPI.endTransaction();

	waitForWrite = 1; 
	writePause(10);
	// since we are writing byte by byte we need to advance address
	// writing byte arrays is unreliable--not sure why
	Address = Address + 1;

}


// not sure why reading and writing byte arrays sometimes fails
// would be a better way if it worked reliably
// maybe a timing issue

/*
uint32_t DBase::saveRecord() {
	
	// we absoutely must have TotalRecords otherwise
	// we will not know where the first writable address
	// is, user can call this to manage the small performance
	// from this call, but if they forget to call it. it must be called
	// before we start writing
	// note that the performance hit is usually 20microseconds
	
	//DebugData(778);

	if (!ReadComplete){
		//unsigned long st = micros();
		Serial.print("readTotalRecords not done. so doing it now...");
		//Serial.println(micros() - st);
		
		readTotalRecords();
		//Serial.print("readTotalRecords ");Serial.println(TotalRecords);
		ReadComplete = true;
	}
	if (NextWritableAddress >= CARD_SIZE){
		// no room
		return CARD_SIZE;
	}	
	
	Address = NextWritableAddress;
	//DebugData(800);
	
	Address = CurrentRecord * RecordLength;
	
	
	for (i= 0; i < FieldCount; i++){
		
		if (DataType[i] == DT_U8){
			a1Byte[0] = *u8data[i];
			//WriteData(*u8data[i]);
			//Serial.print("1504 field: ");
			//Serial.print(i);
			//Serial.print(", Address "); Serial.print(Address);
			//Serial.print(", saving byte "); Serial.println(a1Byte[0]);
			//WriteData(*u8data[i]);
			saveField(a1Byte, i);
		}
		else if (DataType[i] == DT_INT){			
			B4ToBytes(a4Bytes, *i16data[i]);
			saveField(a4Bytes, i);
		}
		else if (DataType[i] == DT_I16){
			B2ToBytes(a2Bytes, *i16data[i]);
			saveField(a2Bytes, i);
		}
		else if (DataType[i] == DT_U16){
			B2ToBytes(a2Bytes, *u16data[i]);
			saveField(a2Bytes, i);
		}
		else if (DataType[i] == DT_I32){
			B4ToBytes(a4Bytes, *i32data[i]);
			saveField(a4Bytes, i);
		}
		else if (DataType[i] == DT_U32){
			B4ToBytes(a4Bytes, *u32data[i]);
			saveField(a4Bytes, i);
		}
		else if (DataType[i] == DT_FLOAT){		
			FloatToBytes(a4Bytes, *fdata[i]);
			
			if (CurrentRecord == 9){
				Serial.print("Float data: ");
				Serial.print(*fdata[i]);Serial.print(", ");	
				Serial.print(fieldname[i]);Serial.print(", ");
				Serial.print(*fdata[i]);Serial.print(", ");		
				Serial.print(a4Bytes[0]);Serial.print(", ");
				Serial.print(a4Bytes[1]);Serial.print(", ");
				Serial.print(a4Bytes[2]);Serial.print(", ");
				Serial.println(a4Bytes[3]);	
			}
			
			saveField(a4Bytes, i);
		}	
		else if (DataType[i] == DT_DOUBLE){		
			DoubleToBytes(a8Bytes, *ddata[i]);
			saveField(a8Bytes, i);
		}		
	
		else if (DataType[i] == DT_CHAR){
				
			strcpy(buf,cdata[i]);
			for (j = 0; j < MAXDATACHARLEN; j ++){
				WriteData(buf[j]);	
			}		
			// Serial.println("");
		}
	}


	Address = TempAddress;
	
	// now that record is written, bump the address to the next writable
	// address
	Records++;
	CurrentRecord++;
	TotalRecords++;
	
	Address = NextWritableAddress + RecordLength;
	NextWritableAddress	= Address;
	
	//DebugData(865);
	return CurrentRecord;
	
}

uint32_t DBase::saveHeader() {


	if (!ReadComplete){
		//unsigned long st = micros();
		//Serial.print("readTotalRecords not done. so doing it now...");
		//Serial.println(micros() - st);
		
		readTotalRecords();
		//Serial.print("readTotalRecords ");Serial.println(TotalRecords);
		ReadComplete = true;
	}
	
	
	
	
	TempAddress = Address;
	//Serial.print("TempAddress ");Serial.println(TempAddress);
	//Serial.println(Address);
	// Serial.print("Address ");Serial.println(Address);
	
	
	for (i= 0; i < Header_FieldCount; i++){
		//Serial.print("Address ");Serial.print(Address);
		//Serial.print(", Writing header for: ");Serial.print(Header_FieldName[i]);
		
		if (Header_DataType[i] == DT_U8){
			//Serial.print(", 864 ");Serial.print(*u8hdata[i]);Serial.print(", ");
			//Serial.println(*u8hdata[i]);
			a1Byte[0] = *u8hdata[i];
			saveField(a1Byte, i);
		}
		else if (Header_DataType[i] == DT_INT){
		
			//Serial.print(", 869 ");Serial.print(*inthdata[i]);Serial.print(", ");
			//Serial.print(a4Bytes[0]);Serial.print(", ");
			//Serial.print(a4Bytes[1]);Serial.print(", ");
			//Serial.print(a4Bytes[2]);Serial.print(", ");
			//Serial.println(a4Bytes[4]);
			

			B4ToBytes(a4Bytes, *inthdata[i]);
			saveField(a4Bytes, i);
			
		}
		else if (Header_DataType[i] == DT_I16){
			//Serial.print(", 879 ");Serial.print(*i16hdata[i]);Serial.print(", ");
			
			//Serial.print(a4Bytes[0]);Serial.print(", ");
			//Serial.println(a4Bytes[1]);

			
			B2ToBytes(a2Bytes, *i16hdata[i]);
			saveField(a2Bytes, i);
		}
		else if (Header_DataType[i] == DT_U16){
			//Serial.print(", 885 ");Serial.print(*u16hdata[i]);Serial.print(", ");
			//			Serial.print(a4Bytes[0]);Serial.print(", ");
			//Serial.println(a4Bytes[1]);
			B2ToBytes(a2Bytes, *u16hdata[i]);
			saveField(a2Bytes, i);
		}
		else if (Header_DataType[i] == DT_I32){
			//Serial.print(", 892 ");Serial.println(*i32hdata[i]);
			B4ToBytes(a4Bytes, *i32hdata[i]);
			saveField(a4Bytes, i);
		}
		else if (Header_DataType[i] == DT_U32){
			//Serial.print(", 900 ");Serial.println(*u32hdata[i]);
			B4ToBytes(a4Bytes, *u32hdata[i]);
			saveField(a4Bytes, i);
		}
		else if (Header_DataType[i] == DT_FLOAT){
			//Serial.print(", 908 ");Serial.println(*fhdata[i]);
			FloatToBytes(a4Bytes, *fhdata[i]);
			saveField(a4Bytes, i);

		}
		else if (Header_DataType[i] == DT_DOUBLE){
			//Serial.print(", 917 ");Serial.println(*dhdata[i]);
			DoubleToBytes(a8Bytes, *dhdata[i]);
			saveField(a8Bytes, i);
		}		
	}


	Address = TempAddress;
	
	// now that record is written, bump the address to the next writable
	// address
	Records++;
	CurrentRecord++;
	TotalRecords++;
	
	Address = CurrentRecord * RecordLength;
	
	//DebugData(865);
	return CurrentRecord;
	
}

void DBase::saveField(uint8_t Data[], uint8_t Field) {
		
	TempAddress = Address;
	
	
	// get the address back to the first byte of the record--need a known starting point
	Address = CurrentRecord * RecordLength;
	//Serial.print("731 saveField start address ");Serial.println(Address);
	Address = Address + FieldStart[Field];
	//Serial.print("733 save address ");Serial.println(Address);
	//Serial.print("field length ");Serial.println(FieldLength[Field]);
	//Serial.print("sizeof data arrat ");Serial.println(sizeof(Data));
		
	SPI.beginTransaction(SPISettings(SPEED_WRITE, MSBFIRST, SPI_MODE0));
	digitalWriteFast(cspin, LOW);
	SPI.transfer(WRITEENABLE); // write instruction
	digitalWriteFast(cspin, HIGH); 
	SPI.endTransaction();
	
	flash_wait_for_write = 1;
	write_pause();
  
 	SPI.beginTransaction(SPISettings(SPEED_WRITE, MSBFIRST, SPI_MODE0));
	digitalWriteFast(cspin, LOW);

	SPI.transfer(WRITE); // write instruction
	SPI.transfer((uint8_t) ((Address >> 16) & 0xFF));
	SPI.transfer((uint8_t) ((Address >> 8) & 0xFF));
	SPI.transfer((uint8_t) (Address & 0xFF));
		
	SPI.transfer(Data, Data, FieldLength[Field]);
	// SPI.transfer(255); // data sheet says to write this...   
	digitalWriteFast(cspin, HIGH);  
	
	SPI.endTransaction();

	flash_wait_for_write = 1; 
	write_pause();

	Address = TempAddress;
	UsedAddress = FieldLength[Field];
  

}

*/

//////////////////////////////////////////////////////////
