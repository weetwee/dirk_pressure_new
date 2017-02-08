/*
it uses an Arduino UNO + a shield with RTC & SD-card (from : www.kibuck.com)
use & compile with : Arduino 1.5.5 (on my MacBook) , or Arduino 1.0.6 on my MacBook Pro
Date and time functions using a DS1307 RTC connected via I2C (use Wire lib)
Rev. 02-mar-2014 : we started with the RTC chip (a DS1307)
Rev. 03-mar-2014 : now with the LCD (I2C) display & Arduino 1.0.1
Rev. 03-mar-2014 : now also working with Arduino 1.5.5
Rev. 03-mar-2014 : first attempt with an SD-card
Rev. 04-mar-2014 : SD-card detect on Pin A2 , SD-card write protection on PIN A3
                   & now with a variable filename (Format (DOS 8+3) : DDMMhhmm.log)
Rev. 06-mar-2014 : now with a rotary encoder knob for the Mode selection
                   (a temporary solution)
                   Mode 0 : 5 min. measurement (10 samples/sec.) <- see further !
                   Mode 1 : 1 hour measurement (1 sample/sec.) <- see further !
                   Mode 2 : 8 hour measurement (a sample every 10 sec.) <- see further !
                   we are not going to use a rotary encoder , but instead 2 push-buttons
                   1 for changing the mode & another one for start/stop the logging to
                   a file
Rev. 07-mar-2014 : now we give a short pulse on PIN 8 (every sample) & adjusted some
                   timings , the short pulse is only for testing purposes (to check the
                   interrupt timer)
                   we changed the timings of the different Modes , deleted the variable
                   mode_delay
                   added counter1 (for the serial line logging)
Rev. 08-mar-2014 : now with max_counters[3] & mode_interrupt_timers[3] + some more
                   comments  + new Mode definitions <- see further for the details !
Rev. 09-mar-2014 : the maximum number for delayMicroseconds() is 16383 !
                   the Git version with hash : b589 was more stable !!!!!
                   now with 47 nF over the button contacts of the 2 switches
                   (debounce delay is now 150 mS.)
Rev. 11-mar-2014 : versions from 10-mar-2014 and later where unstable !!
                   (LOW memory problems ?)
                   we went back to a version of 09-mar-2014 & started with storing all
                   the fixed text stings in flash memory (see PROGMEM) instead of in
                   dynamic SRAM 
                   free dynamic SRAM goes up from 367 to 700 bytes !
                   added sd_init() .....
                   we also print the sampling period with the Mode : ....
Rev. 12-mar-2014 : now we also show the counter (or counter1) + value on the LCD (line 0)
                   + some more constant values in PROGMEM (not all - see further)
                   the value is for the moment the result of : analogRead(A0) ;
                   all fixed strings are now in PROGMEM
Rev. 13-mar-2014 : we replaced . by comma's in the log file (see Excell - Windows) 
                   tried 5 meas./sec. in Mode : 0 (duration is 5 min.)
Rev. 14-mar-2014 : now also with the Firmware Rev. in the log file
                   back to 2 meas./sec. in Mode : 0
                   file extension is now *.csv
                   deleted all the unnecessary code
Rev. 23-mar-2014 : some more fixed strings in PROGMEM , deleted show_counter() was not
                   used anymore
Rev. 24-mar-2014 : now we show a message if the Date/Time is adjusted
Rev. 26-mar-2014 : now we measure real pressures (in Bar)
                   we use an 0 .. 150 psi sensor : Honeywell : SSCDANT150PGAA5
                   (ordered via TME)
                   now the string " bar" is also in PROGMEM
Rev. 27-mar-2014 : some more comments , tried to blink LED on PIN 13 (does not work ?)
                   pin 13 is in use for the SPI interface (see the SD card) !
Rev. 02-apr-2014 : small changes in the *.csv file , this version was given to Oxypoint
                   for evaluation / testing (02-apr-2014)
                   after the discussion at Oxypoint we again changed the *.csv format
                   (now without counter) & we also changed the text on the LCD during
                   the Modes - we now display the sampling rate & the total file
                   log. duration
Rev. 04-apr-2014 : now we show the time when the file logging was stopped
                   Mode 0 : 2 measurements/sec. during 5 minutes -> 600 data points
                   Mode 1 : 1 measurement/sec. during 1 hour -> 3600 data points
                   Mode 2 : every 5 sec. a measurement during 8 hours -> 5760 data points
Rev. 06-apr-2014 : corrected some typo's
Rev. 11-apr-2014 : new de-bouncing algorithm
Rev. 13-apr-2014 : corrected some typo's in the setup() function
Rev. 18-apr-2014 : a new value is now the average of 5 measuring points (during 100 mS.)
                   this version is now on demo at Oxypoint (from friday 25-apr-2014 till ......)
                   for testing purposes.
Rev. 30-apr-2014 : added myFile.flush() after every group of myFile.print....
Rev. 01-may-2014 : the variable "value" is now of type "volatile" , and the average of 5 data points is
                   now over a period of 10 mS. instead of 100 mS.
Rev. 23-may-2014 : the file name is now MMDDHHMM .. instead of DDMMHHMM (request by Oxypoint) , average period is
                   now 50 mS.
                   suggestion : use the median of 5 points instead of the average ?
                   added stop_mode (see the end of the log file) , manual or automatic stop
Rev. 13-jan-2017 : added : mode 3 : a measurement every 5 sec. during 8 days
Rev. 16-jan-2017 : now we start in mode : 3 with the logging activated (if we have an SD-card inserted).
Rev. 23-jan-2017 : counter & max_counter is now : unsigned long , max_counters[] is now : prog_uint32_t
                   changed : mark_begin()
Rev. 30-jan-2017 : now max_counter is correct for mode 3 , solved the bug of the rubbish characters
                   at the end of a log. file (because you should use strcpy_P(....)  instead of strcpy(...) )
Rev. 31-jan-2017 : try to give the file the correct date & time for the file browser (but does it work ?)
Rev. 08-feb-2017 : created an entry on www.github.com
*/

#include <Wire.h>
#include "RTClib.h"
#include <SPI.h>
#include <SD.h>
#include <TimerOne.h>
#include <avr/pgmspace.h>

#include <LiquidCrystal_I2C.h>

#define DS1307_ADDRESS 0x68
#define DS1307_CONTROL_REGISTER 0x07

// we put all the fixed text string in FLASH memory instead of in dynamic SRAM

prog_char title_string[] PROGMEM = "Pressure - Logging" ;
prog_char rev1_string[] PROGMEM = "Rev. 31-jan-2017" ;
prog_char email_string[] PROGMEM = "email : w2@skynet.be" ;
prog_char no_sd_card[] PROGMEM = "No SD-Card inserted" ;
prog_char wr_protected[] PROGMEM = "SD : Write protected" ;
prog_char card_ready[] PROGMEM = "SD-Card : Ready" ;
prog_char sd_init_failed[] PROGMEM = "SD-Card Init. failed" ;
prog_char mode_0[] PROGMEM = "0.5 sec. - 5 min." ;
prog_char mode_1[] PROGMEM = "1.0 sec. - 1 hour" ;
prog_char mode_2[] PROGMEM = "5.0 sec. - 8 hours" ;
prog_char cannot_open[] PROGMEM = "Cannot open a file" ;
prog_char end_logging[] PROGMEM = "Stopped at " ;
prog_char mode_str[] PROGMEM = "#Mode : " ;
prog_char start_time[] PROGMEM = "#Start time/date : ";
prog_char stop_time[] PROGMEM = "#Stop time/date : ";
prog_char rec_str[] PROGMEM = "Received : " ;
prog_char file_str[] PROGMEM = "File : " ;
prog_char period_str[] PROGMEM = "#Sampling rate : " ;
prog_char sec_str[] PROGMEM = " sec." ;
prog_char firmware_str[] PROGMEM = "#Firmware : " ;
prog_char date_time_adj[] PROGMEM = "Date/Time adjusted" ;
prog_char bar_str[] PROGMEM = " bar" ;
prog_char man_stop[] PROGMEM = " (manual stop)" ;
prog_char auto_stop[] PROGMEM = " (automatic stop)" ;
prog_char mode_3[] PROGMEM = "5.0 sec. - 8 days" ;

PROGMEM const char *string_table[] = {
 title_string,    // index 0
 rev1_string,     // index 1
 email_string,    // index 2
 no_sd_card,      // index 3 
 wr_protected,    // index 4
 card_ready,      // index 5
 sd_init_failed,  // index 6
 mode_0,          // index 7
 mode_1,          // index 8
 mode_2,          // index 9
 cannot_open,     // index 10
 end_logging,     // index 11
 mode_str,        // index 12
 start_time,      // index 13
 stop_time,       // index 14
 rec_str,         // index 15
 file_str,        // index 16
 period_str,      // index 17
 sec_str,         // index 18
 firmware_str,    // index 19
 date_time_adj,   // index 20
 bar_str,         // index 21
 man_stop,        // index 22
 auto_stop,       // index 23
 mode_3           // index 24
} ;

RTC_DS1307 rtc;

char rec_time_code[16] ; // the received string with a new timestamp (10 digits)
int pos ; // position in the received string : rec_time_code[]
long new_time ; // the timestamp as a long integer

DateTime now ;
byte prev_sec , act_sec ;

// for the LCD (I2C version from www.iprototype.nl)

#define lcd_adr 0x27 // I2C address
#define nr_lines 4 // LCD has 2 or 4 lines
#define nr_char_line 20 // each line can contain up to 16 or 20 char.

LiquidCrystal_I2C lcd(lcd_adr,nr_char_line,nr_lines) ;

char lcd_mess[21] ; // for a general message & for a message line on the LCD
char date_time[20] ; // in the format : hh:mm:ss DD-MM-YYYY

File myFile ;
unsigned long counter ; // for the log file
unsigned long counter1 ; // for the Serial logging

// some Mode parameters :

// Mode 0 : 2 measurements per sec. during 5 minutes
// Mode 1 : 1 measurement per sec. during 1 hour
// Mode 2 : a measurement every 5 sec. during 8 hours
// Mode 3 : a measurement every 5 sec. during 8 days

PROGMEM prog_uint32_t max_counters[] = {600 , 3600 , 5760 , 138240} ;
unsigned long max_counter ;
PROGMEM prog_uint32_t mode_interrupt_times[] = {500000 , 1000000 , 5000000 , 5000000} ;

#define SD_DETECT A2 // used for the SD-card detection
#define SD_WRITE_PROTECTED A3 // used to check if the SD-card is write protected

byte try_to_open_file ;
byte try_to_stop_logging ;

byte mode ; // for the moment we have Mode 0 , 1 , 2 , 3

#define PULSE 8 // we give a short pulse on PIN 8 (every sample or timer interrupt)
#define CS 10

volatile float value ; // the value we measured

byte log_data ;
byte stop_mode ; // default is 1 , automatic stop

/************************************************
Convert the string lcd_mess to exactly 20 char. *
to avoid flickering on the LCD                  *
we do not want to use clear_lcd_line()          *
but lcd.setCursur(0,....)                       *
************************************************/

void send_mess_20char(int l) // 08-mar-2014

{
 int len ;

 lcd.setCursor(0,l) ;
 
 len=strlen(lcd_mess) ;
  
 if (len >= nr_char_line) {
  if (len > nr_char_line) lcd_mess[nr_char_line]='\0' ; // we truncate it
  lcd.print(lcd_mess) ;
  return ;
 }
 
 do {
  strcat(lcd_mess," ") ;
 } while (strlen(lcd_mess) < 20) ;

 lcd.print(lcd_mess) ;
}

/******************************
Clear an LCD line             *
l = 0 ... nr_lines-1          *
******************************/

void clear_lcd_line(int l) // 08-mar-2014

{
 if (l >= nr_lines) return ; // invalid line , should not occur

 sprintf(lcd_mess," ") ;
 send_mess_20char(l) ;

 lcd.setCursor(0,l) ;
}

/***********************************
Mark the START of the logging      *
mode + date/time info              *
***********************************/

void mark_begin() // 31-jan-2017

{
 float period ;

 if (myFile) {
  strcpy_P(lcd_mess,(char*)pgm_read_word(&(string_table[12]))) ; // Mode :
  myFile.print(lcd_mess) ;
  myFile.print(mode) ;
  myFile.print(" , ") ;
  if (mode == 0) strcpy_P(lcd_mess,(char*)pgm_read_word(&(string_table[7]))) ; else // mode 0
   if (mode == 1) strcpy_P(lcd_mess,(char*)pgm_read_word(&(string_table[8]))) ; else // mode 1
    if (mode == 2) strcpy_P(lcd_mess,(char*)pgm_read_word(&(string_table[9]))) ; else // mode 2
     if (mode == 3) strcpy_P(lcd_mess,(char*)pgm_read_word(&(string_table[24]))) ; // mode 3
  myFile.println(lcd_mess) ;
  period=(float)pgm_read_dword(mode_interrupt_times+mode)/1000000.0 ;
  // Serial.print("Period : ") ;
  // Serial.print(period) ;
  // Serial.println(" sec.") ;
  strcpy_P(lcd_mess,(char*)pgm_read_word(&(string_table[17]))) ; // sampling rate
  myFile.print(lcd_mess) ;
  Serial.print(lcd_mess) ;
  dtostrf(period,3,1,lcd_mess) ;
  myFile.print(lcd_mess) ;
  Serial.print(lcd_mess) ;
  strcpy_P(lcd_mess,(char*)pgm_read_word(&(string_table[18]))) ; // sec.
  myFile.println(lcd_mess) ;
  Serial.println(lcd_mess) ;
  strcpy_P(lcd_mess,(char*)pgm_read_word(&(string_table[19]))) ; // #Firmware
  myFile.print(lcd_mess) ;
  strcpy_P(lcd_mess,(char*)pgm_read_word(&(string_table[1]))) ; // Firmware rev.
  myFile.println(lcd_mess) ;
  strcpy_P(lcd_mess,(char*)pgm_read_word(&(string_table[13]))) ; // start time
  myFile.print(lcd_mess) ;
  get_act_date_time() ;
  myFile.println(date_time) ;
  myFile.flush() ;
  counter=0 ;
  log_data=1 ;
 }
}

/***********************************
Mark the END of the logging        *
only date/time info                *
& manual or automatic stop         *
***********************************/

void mark_end() // 30-jan-2017

{
 if (myFile) {
  log_data=0 ;
  strcpy_P(lcd_mess,(char*)pgm_read_word(&(string_table[14]))) ; // stop time
  myFile.print(lcd_mess) ;
  get_act_date_time() ;
  myFile.print(date_time) ;
  if (stop_mode) strcpy_P(lcd_mess,(char*)pgm_read_word(&(string_table[23]))) ;
   else strcpy_P(lcd_mess,(char*)pgm_read_word(&(string_table[22]))) ;
  myFile.println(lcd_mess) ;
  myFile.flush() ;
 } 
}

/*******************************************
The welcome text on the LCD and/or console *
*******************************************/

void welcome() // 11-mar-2014

{
 strcpy_P(lcd_mess,(char*)pgm_read_word(&(string_table[0]))) ; // title
 Serial.println(lcd_mess) ;
 send_mess_20char(0) ;

 strcpy_P(lcd_mess,(char*)pgm_read_word(&(string_table[1]))) ; // software rev.
 send_mess_20char(1) ;
 Serial.println(lcd_mess) ;
 
 if (nr_lines == 4) { // we have a display with 4 lines
  strcpy_P(lcd_mess,(char*)pgm_read_word(&(string_table[2]))) ; // show the e-mail
  send_mess_20char(nr_lines-1) ;
  my_delay(3000) ;
  if (sd_detect()) {
   strcpy_P(lcd_mess,(char*)pgm_read_word(&(string_table[3]))) ; // no SD card detected
  } else {
     if (write_protected()) strcpy_P(lcd_mess,
      (char*)pgm_read_word(&(string_table[4]))) ; // SD is write protected
       else strcpy_P(lcd_mess,(char*)pgm_read_word(&(string_table[5]))) ; // SD is ready
    }
  send_mess_20char(nr_lines-1) ;
  my_delay(2000) ;
 }  
}

/********************************************************
Set the external clock to 32.768 Hz. (square wave)      *
the interface shield needs an extra pull-up resistor    *
from PIN SQ to 3.3 Volt                                 *
********************************************************/

void set_external_clock() // 24-mar-2014

{
 uint8_t val ;
 
 val=0x13 ; // check with the data-sheet - seems OK.
 
 Wire.beginTransmission(DS1307_ADDRESS) ;
 Wire.write(DS1307_CONTROL_REGISTER) ;
 Wire.write(val) ;
 Wire.endTransmission() ;
}

/********************************************************************
Clear the string that's used for receiving a new date/time setting  *
********************************************************************/

void clear_rec_time_code() // 02-mar-2014

{
 memset(&rec_time_code[0],'\0',sizeof(rec_time_code)) ;
 pos=0 ; 
}

/************************
Put the backlight ON    *
************************/

void lcd_on() // 28-may-2012

{
 lcd.backlight() ;
 lcd.display() ;
}

/*********************************************
Detect if an SD-card is inserted ?           *
checked on PIN A2 , the CD PIN is the output *
from the SD-card reader                      *
1 = No card is inserted                      *
0 = an SD-card is inserted                   *
*********************************************/

int sd_detect() // 04-mar-2014
 
{
 return(digitalRead(SD_DETECT)) ; 
}

/*********************************************
Detect if the SD-card is write protected ?   *
checked on PIN A3, the WP PIN is the output  *
from the SD-card reader                      *
1 = the card is write protected              *
0 = the SD-card is writable                  *
*********************************************/

int write_protected() // 04-mar-2014

{
 return(digitalRead(SD_WRITE_PROTECTED)) ; 
}

/***************************************
Init of the SD card                    *
***************************************/

void sd_init() // 11-mar-2014

{
 if (!SD.begin(CS)) { // C/S of the SD-card to PIN 10
  strcpy_P(lcd_mess,(char*)pgm_read_word(&(string_table[6]))) ; // SD init. failed
  Serial.println(lcd_mess) ;
  send_mess_20char(1) ;
 } 
}

/*****************************************
The Arduino setup() function             *
*****************************************/

void setup () { // 31-jan-2017
  
 Serial.begin(9600);
 Wire.begin();
 
 pinMode(SD_DETECT,INPUT_PULLUP) ; // activate the pull-up resistor
 pinMode(SD_WRITE_PROTECTED,INPUT_PULLUP) ;
 
 pinMode(2,INPUT_PULLUP) ; // for the mode selection (interrupt 0)
 pinMode(3,INPUT_PULLUP) ; // for the start/stop of the file logging (interrupt 1)
 
 pinMode(PULSE,OUTPUT) ; // only for test purposes
 digitalWrite(PULSE,LOW) ; // set the line LOW
 
 pinMode(CS,OUTPUT) ;

 // pinMode(LED,OUTPUT) ;
 // digitalWrite(LED,HIGH) ; // the LED is OFF

 clear_rec_time_code() ;
 
 lcd.init() ;
 lcd_on() ;
 lcd.setCursor(0,0);
 
 welcome() ;
 
 rtc.begin();
 
 set_external_clock() ; // set the external clock to 32.768 Khz.
 
 if (! rtc.isrunning()) {
  Serial.println("*** RTC is NOT running - set DATE/TIME ***");
  // following line sets the RTC to the date & time this sketch was compiled
  rtc.adjust(DateTime(__DATE__, __TIME__));
 }
 
 sd_init() ;
 
 analogReference(DEFAULT) ; // the A/D range is from 0.0 to 5.0 Volt (10 bit) ;
 
 analogRead(A0) ; // a dummy read on the analog input
 delay(10) ;
 analogRead(A0) ; // another dummy read on the analog input
 
 counter=0 ;
 counter1=0 ;
 mode=3 ; // we always start in Mode 0 , now we start in mode 3

 try_to_open_file=1 ; // was 0
 try_to_stop_logging=0 ;
 stop_mode=1 ; // default is automatic stop
 
 get_act_date_time() ;
 
 attachInterrupt(0,set_mode,FALLING) ; // interrupt to change the mode (a switch to PIN 2)
 attachInterrupt(1,start_logging,FALLING) ; // interrupt to start/stop file logging
 // (a switch to PIN 3)
 
 Timer1.initialize(pgm_read_dword(mode_interrupt_times+mode)) ; // 2 samples/sec.
 Timer1.attachInterrupt(my_irq) ;
}

/***********************************************
My delay() in mS.                              *
can be used in an interrupt routine & calls :  *
digitalMicroseconds()                          *
***********************************************/

void my_delay(int ms) // 09-mar-2014

{
 int n ; 
 
 for (n=0 ; n < ms ; n++) delayMicroseconds(1000) ; 
}

/********************************************************************************
Get a new measurement value (now an average of 5 points)                        *
we use an 0 .. 150 psi sensor : Honeywell : SSCDANT150PGAA5 (ordered via TME)   *
suggestion : use the median of 5 points (= point at pos. 2 in the array 0...4)  *
********************************************************************************/

void get_value() // 23-may-2014

{
 int n , sum ;
 
 // average 5 points during a period of 50 mS.
 
 sum=0 ;
 for (n=0 ; n < 5 ; n++) {
  sum=sum+analogRead(A0) ;
  my_delay(10) ; // wait 10 mS.  
 }
 // sum=sum/5.0 ;
 
 value=sum/1024.0 ; // analog input value in Volt
 value=(value-0.5)*30.0/0.8 ; // value in psi (see the Honeywell datasheet)
 value=value*0.0689 ; // psi -> bar conversion
}

/*******************************************
We have a Timer1 interrupt                 *
*******************************************/

void my_irq() // 30-apr-2014

{
 char tot_message[16] ;
 char new_message[16] ;
 
 char val_str[8] ;
 int i , l , n ;
 
 // digitalWrite(LED,!digitalRead(LED)) ; // toggle the LED
 
 noInterrupts() ;
 
 give_pulse() ;
 counter++ ; // the counter for the log file 
 counter1++ ; // the counter for the serial line
 get_value() ;
 dtostrf(value,6,3,val_str) ;
 sprintf(tot_message,"%ld %s",counter1,val_str) ;
 Serial.println(tot_message) ; // show it on the serial console
 if ( (myFile) && (log_data) ) { // write data to the log file
  sprintf(tot_message,"%s",val_str) ;
  // else sprintf(tot_message,"%s",val_str) ;
  // replace . by , in the output string
  l=strlen(tot_message) ;
  for (n=0 ; n < l ; n++) if (tot_message[n] == '.') tot_message[n]=',' ;
  memset(&new_message[0],'\0',sizeof(new_message)) ;
  i=0 ;
  for (n=0 ; n < l ; n++) if (tot_message[n] != ' ') new_message[i++]=tot_message[n] ;
  myFile.println(new_message) ;
  myFile.flush() ;
 }
 interrupts() ;
 // digitalWrite(LED,LOW) ;
}

/*********************************************
Debouncing algorithm : 150 mS.               *            
We wait till the line is again HIGH & stable *
*********************************************/

void wait_end_debounce(byte pin) // 11-apr-2014

{
 byte n , up ;
  
 do {
  up=0 ;
  for (n=0 ; n < 150 ; n++) {
   if (digitalRead(pin)) up++ ;
   delay(1) ;  
  }
 } while (up < 150) ;
 
}

/****************************************************
Set the new value for the interrupt timer for this  *
mode                                                *
****************************************************/

void set_mode() // 13-jan-2017

{
 if (myFile) return ; // not when logging into a file is busy
 noInterrupts() ;
 mode++ ;
 if (mode > 3) mode=0 ;
 Timer1.initialize(pgm_read_dword(mode_interrupt_times+mode)) ;
 
 // wait for the end of the debounce time
 
 wait_end_debounce(2) ;
 
 counter=0 ;
 counter1=0 ;
 
 interrupts() ;
}

/****************************************************
Start the file logging when it is not yet running   *
or stop it when it is already running               *
****************************************************/

void start_logging() // 31-jan-2017

{
 noInterrupts() ;
 
 // wait for the end of the debounce time
 
 wait_end_debounce(3) ;
 
 if (myFile) {
  try_to_stop_logging=1 ; // a log file is already open
  stop_mode=0 ; // manual stop is activated
 } else {
    try_to_open_file=1 ; // try to open a log file
    stop_mode=1 ;
 }
 interrupts() ; 
}

/*****************************************
Show the current mode  & set some        *
parameters for this mode                 *
*****************************************/

void display_mode() // 13-jan-2017

{
 if (mode == 0) strcpy_P(lcd_mess,(char*)pgm_read_word(&(string_table[7]))) ; else
  if (mode == 1) strcpy_P(lcd_mess,(char*)pgm_read_word(&(string_table[8]))) ; else
   if (mode == 2) strcpy_P(lcd_mess,(char*)pgm_read_word(&(string_table[9]))) ; else
    if (mode == 3) strcpy_P(lcd_mess,(char*)pgm_read_word(&(string_table[24]))) ;    
 max_counter=pgm_read_dword(max_counters+mode)  ;
 send_mess_20char(2) ;  
}

/*******************************************
Get the actual date/time                   *
the result is in the string : date_time    *
*******************************************/

void get_act_date_time() // 08-mar-2014

{
 now = rtc.now();
 act_sec=now.second() ;
 
 if (act_sec != prev_sec) {
  sprintf(date_time,"%02dh%02d:%02d %02d-%02d-%04d",now.hour(),now.minute(),
   now.second(),now.day(),now.month(),now.year()) ;
  prev_sec=act_sec ;
 } 
}

/*******************************************
Check if we got a valid new date/time code *
*******************************************/

int good_time_code() // 24-mar-2014

{
 int l , n , nr_digits ;
 
 l=strlen(rec_time_code) ;
 if (l != 10) return(0) ; // not enough characters
 nr_digits=0 ;
 for (n=0 ; n < l ; n++) if (isdigit(rec_time_code[n])) nr_digits++ ;
 if (nr_digits == 10) return(1) ; else return(0) ;
}

/*************************************************
We received a new char. over the serial line     *
put it in the receive string : rec_time_code[]   *
*************************************************/

void add_char() // 11-mar-2014

{
 byte c ;

 c=Serial.read() ;
 if ( (c == 0x0A) || (c == 0x0D) ) { // an end of line marker
  if (good_time_code()) {
   strcpy_P(lcd_mess,(char*)pgm_read_word(&(string_table[15]))) ; // rec_str
   Serial.print(lcd_mess) ;
   Serial.println(rec_time_code) ;
   new_time=atol(rec_time_code) ; // convert to a long integer
   rtc.adjust(new_time+3600) ; // set the clock to the new timestamp
   strcpy_P(lcd_mess,(char*)pgm_read_word(&(string_table[20]))) ; // Date/Time adjusted
   send_mess_20char(0) ;
   my_delay(2000) ;
  }
  clear_rec_time_code() ;
  return ;
 } else if (pos < sizeof(rec_time_code)) rec_time_code[pos++]=c ;
    else clear_rec_time_code() ;
}

/****************************************
Open a new log file for writing         *
as of 23-may-2014 the filename is now : *
MMDDHHMM instead of DDMMHHMM            *
****************************************/

void open_new_file() // 31-jan-2017

{
 char file_name[16] ; // min. is 8 + 3 + 2
 int min_offset ;

 if (myFile) return ; // a log file is already open

 if (sd_detect()) {
  strcpy_P(lcd_mess,(char*)pgm_read_word(&(string_table[3]))) ; // no SD card detected
  send_mess_20char(1) ;
  return ;
 }

 if (write_protected()) {
  strcpy_P(lcd_mess,(char*)pgm_read_word(&(string_table[4]))) ; // SD is write protected
  send_mess_20char(1) ;
  return ;
 } 
  
 // if the log file already exists then we add an extra minute

 now=rtc.now() ;
 SdFile::dateTimeCallback(dateTime) ; // so that the file has the correct date/time in the file browser
 min_offset=0 ;
 do {
  now=rtc.now() ;
  sprintf(file_name,"%02d%02d%02d%02d.csv",now.month(),now.day(),now.hour(),
   now.minute()+min_offset) ;
  min_offset++ ;
 } while (SD.exists(file_name)) ;
 
 myFile=SD.open(file_name,FILE_WRITE) ;
 counter=0 ;
 try_to_open_file=0 ;
 if (myFile) {
  strcpy_P(lcd_mess,(char*)pgm_read_word(&(string_table[16]))) ;  // file_str
  strcat(lcd_mess,file_name) ;
  send_mess_20char(1) ;
  mark_begin() ;
 } else {
    strcpy_P(lcd_mess,(char*)pgm_read_word(&(string_table[10]))) ; // cannot open file
    Serial.print(lcd_mess) ;
    send_mess_20char(1) ;
    Serial.print(" : ") ;
    Serial.println(file_name) ;
    sd_init() ;
   }
}

/**********************************************
Stop the logging on the SD-card               *
**********************************************/

void stop_sd_card_logging() // 31-jan-2017

{
 mark_end() ;
 myFile.close() ;
 strcpy_P(lcd_mess,(char*)pgm_read_word(&(string_table[11]))) ;  // end of card logging
 get_act_date_time() ;
 date_time[8]='\0' ;
 strcat(lcd_mess,date_time) ;
 send_mess_20char(1) ;
 try_to_stop_logging=0 ;
 counter=max_counter ; // force a file logging stop 
}

/********************************************
Get the date/time & display it on the LCD   *
Also display counter (or counter1) & the    *
measured value                              *
********************************************/

void get_show_date_time_values() // 26-mar-2014

{
 char val_to_str[7] ;
 
 get_act_date_time() ; // the result is in the string date_time
 strcpy(lcd_mess,date_time) ;
 send_mess_20char(3) ;
 dtostrf(value,6,3,val_to_str) ;
 if (myFile) {
  sprintf(lcd_mess,"%d ",counter) ;
 } else sprintf(lcd_mess,"%ld ",counter1) ;
 
 strcat(lcd_mess,val_to_str) ;
 strcat_P(lcd_mess,(char*)pgm_read_word(&(string_table[21]))) ; // bar_str 
 send_mess_20char(0) ; // show the actual counter (or counter1)  & the measured value 
}

/******************************************************
Give a short (HIGH) pulse (only for testing purposes) *
******************************************************/

void give_pulse() // 07-mar-2014

{
 digitalWrite(PULSE,HIGH) ;
 delayMicroseconds(10000) ; // pulse has a 10 mS. width
 digitalWrite(PULSE,LOW) ;
}

// Just like the above posts
// after your rtc is set up and working you code just needs:

void dateTime(uint16_t* date, uint16_t* time) { // 31-jan-2017 , does it work ?

 // now = rtc.now();

 // noInterrupts() ;

 // return date using FAT_DATE macro to format fields
 *date = FAT_DATE(now.year(), now.month(), now.day());

  // return time using FAT_TIME macro to format fields
 *time = FAT_TIME(now.hour(), now.minute(), now.second());
 // interrupts() ;
}

/************************
The main Arduino loop() *
************************/

void loop () { // 31-jan-2017
    
 display_mode() ;
 
 if (try_to_open_file) open_new_file() ;
 
 if (myFile) { // we have a file open for logging the data
  do { // wait till the max. measurement time is reached or we want to stop the logging
   get_show_date_time_values() ;
   my_delay(550) ;
   if (try_to_stop_logging) stop_sd_card_logging() ;
  } while (counter < max_counter) ;
  if (myFile) stop_sd_card_logging() ;
 } else get_show_date_time_values() ;
 
 // check for new serial input & 1 sec. wait loop
 
 for (int n=0 ; n < 100 ; n++) {
  if (Serial.available() > 0) add_char() ;
  my_delay(10) ;
 }
}
