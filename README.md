# TeensyDB
Remember the infamous DBase III from the early '80's? Well here it is for your Teensy microcontroller (Arduino / ESP are not supported) and select flash chips. This library is a database system for SPI-based flash memory chips and uses a field/record approach in saving data to a chip. Some advantages of saving data to a flash-type chip are no physical contacts to loose connection, but have very fast writes and reads. This driver can save ~50 bytes in under 1ms. Since this library has no "close" method to finalize a write transation, saved data can be read even if power was lost during data collection. While you can save data to an SD card, the classic open/write/close has a huge overhead the can take in the 100's of ms to execute. This claim is based on open/write/close repeat. Having Open and Close outide the save loop is much faster but risks loosing data in cases of unexpected power loss. 

This library has been tested with
<br>
<b>MCU</b>
<br>
Teensy 3.2
<br>
Teensy 4.0
<br>
<br>

<b>Flash Chips</b>
<br>
Microchip		SST25F040C (https://ww1.microchip.com/downloads/en/DeviceDoc/SST25PF040C-4-Mbit-3.3V-SPI-Serial-Flash-Data-Sheet-DS20005397E.pdf)
<br>
Winbond 		25Q64JVSIQ (https://www.winbond.com/resource-files/w25q64jv%20revj%2003272018%20plus.pdf)

This driver is intended for data acquistion systems where known data is to be stored. Namely because fields must be established before any operations, and that field list must match any existing database schemes on the chip. Data types is stored in fields (similar to a columns in a spreadsheet, and each record of data values are store in rows. You can easily save measurements such as volts in a volt field, temperature in a temp field, etc. Since a known scheme is required, it's not intended for saving video, images, or "random" data. Because of the database scheme, you cannot change the fields unless you erase the chip first.

If you are not familiar with fields and records, fields are the columns, and records are the rows. Similar to:

<table>
  <tr>
    <th>Recordset</th>
    <th>Datapoint</th>
    <th>Time</th>
    <th>Temp1</th>
    <th>Temp2</th>
    <th>Temp2</th>
    
  </tr>
  <tr>
    <td>1</td>
    <td>1</td>
    <td>10:00</td>
    <td>23.4</td>
    <td>45.2</td>
    <td>63.1</td>
  </tr>
  <tr>
    <td>1</td>
    <td>2</td>
    <td>10:01</td>
    <td>23.6</td>
    <td>45.1</td>
    <td>65.4</td>
  </tr>
    <tr>
      <td>1</td>
    <td>3</td>
    <td>10:02</td>
    <td>23.7</td>
    <td>45.0</td>
    <td>67.2</td>
  </tr>
</table>

This driver lets you create fields of specified data types, then in some measurement loop add a new record, save a record, and repeat. As with many flash chips you CANNOT write to an address unless it's in the erased state hence this library requires data to be written and stored contiguous. This driver will find the next available writable address so if you power up your system, and start saving data, you can be sure you will be writing to valid addresses. The field definition process passes pointers into the library so the save process simply looks at the data you already have in memory. This design keeps you from having to populate and save a bunch of fields. One addRecord() call and one saveRecord() call is all that is needed to save your data to the chip.
<br>
<b><h3>Library highlights</b></h3>
1. relatively small footprint
2. very fast write times (approx 32 microseconds per byte)
3. ability to add up to 255 fields
4. ability to add a new record (required for each record save)
5. ability to save a record with a single call
6. ability to goto a record
7. ability to read the data out of a field 
8. ability to get total records so save can start at a valid address
10. ability to save bytes, ints, uint32_t, floats, char[fixed_length], doubles, more... But sorry STRING is not supported. 
11. ability to get chips stats (JEDEC codes, and used space)
12. ability to erase a sector or the entire chip (caution: the chip requires contiguous memory for writing so memory in the middle cannot only be erased)
13. Only 1 field scheme is allow between chip erases
14. a concept of a field called "RecordSet" could be used to distinguish one set of readings from another--similar to a file number
15. this library writes data to the chip byte by byte and not byte arrays. This does impede performance, but improves write reliability.
<br>
<b><h3>Pin Connection</b></h3>
<table>
  <tr>
    <th>flash chip</th>
    <th>CE</th>
    <th>SO/SIO1</th>
    <th>WP</th>
    <th>VSS</th>
    <th>VDD</th>
    <th>HOLD</th>
    <th>SCK</th>
    <th>SI/SIO0</th>
  </tr>
  <tr>
    <th>Teensy</th>
    <th>6 (user selection)</th>
    <th>12 (MISO)</th>
    <th>3v3</th>
    <th>GND</th>
    <th>3v3</th>
    <th>3v3</th>
    <th>13 (SCK)</th>
    <th> 11 (MOSI)</th>
  </tr>
</table>
<br>  
<b><h3>General implementation</b></h3>
<br>
1. include the library
<br>
#include "TeensyDB.h"
<br>
<br>
2. create the chip object
<br>
TeensyDB YOUR_OBJECT_NAME(THE_CHIP_SELECT_PIN);
<br>
<br>
3. create variables
<br>
float MyVolts = 0.0;
<br>
int MyVoltsID = 0;
<br>
uint32_t LastRecord = 0, i = 0;
<br>
<br>
4. In setup, create data fields
<br>
MyVoltsID = YOUR_OBJECT_NAME.addField("Volts", &MyVolts);
<br>
<br>

5. In setup, get the last writable record
<br>
LastRecord = YOUR_OBJECT_NAME.findFirstWritableRecord();
<br>
YOUR_OBJECT_NAME.gotoRecord(LastRecord);
<br>
<br>
6. In some measurement loop
<br>
MyVolts = analogRead(A0);
<br>
<br>
7. Add a new record
<br>
YOUR_OBJECT_NAME.addRecord();
<br>
<br>
8. Save the record
<br>
YOUR_OBJECT_NAME.saveRecord();
<br>
<br>
9. when you are ready to read the data...
<br>
LastRecord = YOUR_OBJECT_NAME.getLastRecord();
<br>
for (i = 1; i <= LastRecord; i++) {
<br>
&nbsp Serial.print("Record: ");
<br>
&nbsp Serial.print(i);
<br>
&nbsp Serial.print(" - ");
<br>
&nbsp Serial.print(YOUR_OBJECT_NAME.getField(MyVolts, MyVoltsID ));
<br>
&nbsp Serial.print(", ");
<br>
}
<br>
<br>
<br>
Performance comparison between 3 different storage options 1) flash chip using this library 2) standard SD card using SdFat 3) flash chio using LittleFS. Users can chose when to close files with SD cards and LittleFS, either close after all data is collected, or close after each datapoint is collected. There are several cases that require the latter, namely when power down is unpredictable in which results in data loss due to lack of file closure. Since this library has no concept of opening and closing, there is no chance of lost data in the even of an unplanned loss of power.

The fastest performance is with a flash chip and LittleFS in a open once / close once scenario. Howerver this configuration becomes very slow when open / close is performed for each data write. SD cards are fast again only in open / close once scenarios. This library offers high performance withought sacrificing data write integridy.
<br>
<br>
![header image](https://raw.github.com/KrisKasprzak/DBase/master/Images/PerformanceComparison2.jpg)
<br>
<br>
Testing shows very reliable record writes and reads. This images shows 500,000 writes and reads with no issues.

![header image](https://raw.github.com/KrisKasprzak/DBase/master/Images/SaveReliability.jpg)

This image shows the inspiration for developing this library. Some 55 bytes are being written every second, and on occassion, the SD write proceess corrupts data. Notice the incorrect data highlighted. This error rarely occurrs (estimated at 1 data point / 10,000) and is not reproducable but not acceptable. During testing and usage, this library has written 500 mb bytes without losing 1 bit.

![header image](https://raw.github.com/KrisKasprzak/DBase/master/Images/SD_DataFail.jpg)

