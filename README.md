# DBase
Remember the infamous DBase III? Well here it is for your MCU's (tested with Teensy and W25Q64JVSSIQ). This library is a database system for SPI-based flash memory chips intended for microcontrollers like the mighty Teensy. This database driver uses a field/record approach in saving data to a chip. While you can save data to and SD card, the classic open/write/save has a huge overhead the can take in the 100's of ms to execute. This driver can save ~50 bytes in under 2ms. Data on the chip can be downloaded to an SD card for portable transfer to a PC. 

https://www.winbond.com/resource-files/w25q64jv%20revj%2003272018%20plus.pdf

This driver is intended for data acquistion systems where known data is to be stored. As it uses a field/record approach, data variables are stored in fields, and each measurement is stored as a record. The intent is to save measurements such as volts in a volt field, temperature in a temp field, etc. Hence, it's not intended for saving video, images, or "random" data. 

If you are not familiar with fields and records, fields are the columns, and records are the rows. Similar to:

<table>
  <tr>
    <th>Record</th>
    <th>Time</th>
    <th>Temp1</th>
    <th>Temp2</th>
    <th>Temp2</th>
    
  </tr>
  <tr>
    <td>1</td>
    <td>10:00</td>
    <td>23.4</td>
    <td>45.2</td>
    <td>63.1</td>
  </tr>
  <tr>
    <td>2</td>
    <td>10:01</td>
    <td>23.6</td>
    <td>45.1</td>
    <td>65.4</td>
  </tr>
    <tr>
    <td>3</td>
    <td>10:02</td>
    <td>23.7</td>
    <td>45.0</td>
    <td>67.2</td>
  </tr>
</table>

This driver lets you create fields of specified data types, then in some measurement loop add a new record, save a record, and repeat. As with many flash chips you CANNOT write to an address unless it's in the erased state. This driver will find the next available writable address so if you power up your system, and start saving data, you can be sure you will be writing to valid addresses. The field definition process passes pointers into the library so the save process simpply looks at the data you already have in memory. This design keeps you from having to save a bunch of fields and pay the save performance hit. One addRecord() call and one saveRecord() call is all that is needed to save your data to the chip. When creating fields, the library uses pointers to automatically get the data for storage.
<br>
<b><h3>Library highlights</b></h3>
1. relatively small footprint
2. very fast write times (32 microseconds per byte)
3. ability to add up to 255 fields
4. ability to add a new record (required for each record save)
5. ability to save a record with a single call
6. ability to goto a record
7. ability to read the data out of a field 
8. ability to get total records so save can start at a valid address
10. ability to save bytes, ints, longs, floats, char[fixed_length], doubles, more... But sorry STRING is not supported. 
11. ability to get chips stats (JDEC#, and used space)
12. ability to erase a sector or the entire chip
13. Only 1 field scheme is allow between chip erases
<br>
<b><h3>Library status</b></h3>
1. works and tested with Winbond W25Q64JVSSIQ<br>
2. can write ~50 bytes in 1.5 ms<br>
3. Library wires byte by byte and not byte arrays for improved write reliability, tested by writing 4mb with zero loss of data<br>
<br>
<b><h3>General implementation</b></h3>
<br>
1. include the library
<br>
#include "DBase.h"
<br>
<br>
2. create the chip object
<br>
DBase YOUR_CHIP_OBJECT(THE_CHIP_SELECT_PIN);
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
MyVoltsID = SSD.addField("Volts", &MyVolts);
<br>
<br>

5. In setup, get the last writable record
<br>
LastRecord = YOUR_CHIP_OBJECT.findLastRecord();
<br>
YOUR_CHIP_OBJECT.gotoRecord(LastRecord);
<br>
<br>
6. In some measurement loop
<br>
MyVolts = analogRead(A0);
<br>
<br>
7. Add a new record
<br>
YOUR_CHIP_OBJECT.addRecord();
<br>
<br>
8. Save the record
<br>
YOUR_CHIP_OBJECT.saveRecord();
<br>
<br>
9. when you are ready to read the data...
<br>
LastRecord = SSD.getLastRecord();
<br>
for (i = 1; i <= LastRecord; i++) {
<br>
&nbsp Serial.print("Record: ");
<br>
&nbsp Serial.print(i);
<br>
&nbsp Serial.print(" - ");
<br>
&nbsp Serial.print(YOUR_CHIP_OBJECT.getField(MyVolts, MyVoltsID ));
<br>
&nbsp Serial.print(", ");
<br>
}
<br>
<br>
<br>
Testing shows very reliable record writes and reads. This images shows 500,000 writes and reads with no issues.

![header image](https://raw.github.com/KrisKasprzak/DBase/master/Images/SaveReliability.jpg)

