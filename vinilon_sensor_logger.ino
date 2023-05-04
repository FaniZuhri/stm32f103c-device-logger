// include the SD library:
#include <SPI.h>
#include <SD.h>
#include <RTClock.h>
#include <String.h>
extern "C"{
  #include "scp.h"
  #include "userdefine.h"
  // #include "sdcard.h"
};

int sd_card_init(void);
int sd_create_folder(void);
int sd_create_file(void);
int cmd_detect_sdcard(uint8_t *buf);
int cmd_get_curr_time(uint8_t *buf);
int cmd_set_curr_time(uint8_t *timestamp);
int cmd_start_logging(uint8_t *buf);
void log_sensor_val(uint16_t *sensor1, uint16_t *sensor2);
void log_sensor_start(void);
void wait_uart_received(void);

// SCP Props
const struct scp_cmd_tbl_s logger_cmd_tbl_s[CMD_TBL_SIZE] =
{
  //Command code      get functions       set functions
	{'D',               cmd_detect_sdcard,	    NULL},
  {'T',               cmd_get_curr_time,   cmd_set_curr_time},
  {'S',               NULL,               cmd_start_logging},
};

// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;

// File props
File myFile;
char folder_name[8];
char file_name[16];
char log_entry[LOG_ENTRY_SIZE];

// RTC props
RTClock rtclock (RTCSEL_LSE); // initialise
time_t time_val;
uint8_t day_time, hour_time, mon_time;

//Create a place to hold the incoming message
extern uart_handler_t uart_gui_hndl;
char msg[MAX_MESSAGE_LENGTH], valid_res_buf[VALID_RES_BUF_SIZE];
unsigned int msg_pos = 0;

char *data_received;

uint8_t sd_card_detected = 0;
uint8_t uart_received = 0;
uint8_t log_started = 0;
uint8_t log_stopped = 0;
uint16_t sensor1_val, sensor2_val;
int status;


void wait_uart_received(void){
  while (Serial.available() > 0)
  {
    if (uart_received) break;

    //Read the next available byte in the serial receive buffer
    char inByte = Serial.read();

    //Message coming in (check not terminating character) and guard for over message size
    if ( inByte != '\n' && (msg_pos < MAX_MESSAGE_LENGTH - 1) )
    {
      //Add the incoming byte to our message
      msg[msg_pos] = inByte;
      msg_pos++;
    }
    //Full message received...
    else
    {
      //Add null character to string
      msg[msg_pos] = '\0';

      //Reset for the next message
      msg_pos = 0;
      uart_received = 1;
      break;
    }
  }
}

int sd_card_init(void){

  Serial.println("\nInitializing SD card...");

  // we'll use the initialization code from the utility libraries
  // since we're just testing if the card is working!
  if (!card.init(SPI_HALF_SPEED, PA4)) {
    Serial.println("initialization failed. Things to check:");
    Serial.println("* is a card inserted?");
    Serial.println("* is your wiring correct?");
    Serial.println("* did you change the chipSelect pin to match your shield or module?");
    // while (1);
    return -1;
  } else {
    Serial.println("Wiring is correct and a card is present.");
  }

  // print the type of card
  Serial.println();
  Serial.print("Card type:         ");
  switch (card.type()) {
    case SD_CARD_TYPE_SD1:
      Serial.println("SD1");
      break;
    case SD_CARD_TYPE_SD2:
      Serial.println("SD2");
      break;
    case SD_CARD_TYPE_SDHC:
      Serial.println("SDHC");
      break;
    default:
      Serial.println("Unknown");
  }

  // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
  if (!volume.init(card)) {
    Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
    // while (1);
    return -1;
  }

  Serial.print("Clusters:          ");
  Serial.println(volume.clusterCount());
  Serial.print("Blocks x Cluster:  ");
  Serial.println(volume.blocksPerCluster());

  Serial.print("Total Blocks:      ");
  Serial.println(volume.blocksPerCluster() * volume.clusterCount());
  Serial.println();

  // print the type and size of the first FAT-type volume
  uint32_t volumesize;
  Serial.print("Volume type is:    FAT");
  Serial.println(volume.fatType(), DEC);

  volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
  volumesize *= volume.clusterCount();       // we'll have a lot of clusters
  volumesize /= 2;                           // SD card blocks are always 512 bytes (2 blocks are 1KB)
  Serial.print("Volume size (Kb):  ");
  Serial.println(volumesize);
  Serial.print("Volume size (Mb):  ");
  volumesize /= 1024;
  Serial.println(volumesize);
  Serial.print("Volume size (Gb):  ");
  Serial.println((float)volumesize / 1024.0);

  // Serial.println("\nFiles found on the card (name, date and size in bytes): ");
  // root.openRoot(volume);

  // // list all files in the card with date and size
  // root.ls(LS_R | LS_DATE | LS_SIZE);

  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    while (1);
  }

  return 0;
}

int cmd_detect_sdcard(uint8_t *buf){
  Serial.println("Detecting SD Card...");
  sd_card_init();
  sd_card_detected = 1;
  return VALID_CMD;
}

int cmd_get_curr_time(uint8_t *buf){
  time_t timestamp;

  timestamp = rtclock.getTime();

  Serial.println(timestamp);

  return VALID_CMD;
}

int cmd_set_curr_time(uint8_t *timestamp){
  Serial.println("Set time to RTC");

  time_val = atoi((char *)timestamp);
  Serial.println(time_val);

  rtclock.setTime(time_val);
  
  return VALID_CMD;
}

int cmd_start_logging(uint8_t *buf){
  if (*buf == '1') log_started = 1;
  if (*buf == '0') log_stopped = 1;
  return VALID_CMD;
}

int sd_create_folder(void){
  sprintf(folder_name, "%02d%02d", day_time, mon_time);
  // const char *folder_name_ptr = folder_name;

  if (!SD.exists(folder_name)) {
    Serial.print("Create folder name ");
    Serial.println(folder_name);
    SD.mkdir((const char *)folder_name);
    // SD.mkdir("abc");
  }

  return 0;
}

int sd_create_file(void){
  sprintf(file_name, "/%s/%02d.txt", folder_name, hour_time);

  if (!SD.exists(file_name) || !myFile){
    Serial.print("Create file ");
    Serial.println(file_name);
    myFile = SD.open((const char *)file_name, FILE_WRITE);
  }

  return 0;
}

void log_sensor_start(void){
  uint32_t timestamp;
  uint16_t sensor1, sensor2;
  uint8_t rtc_day, rtc_hour, rtc_mon;

  if (!sd_card_detected) {
    if (sd_card_init() != 0) {
      Serial.println("SD Card not detected!");
      return;
    }
    sd_card_detected = 1;
    delay(1000);
  }

  timestamp = rtclock.getTime();
  rtc_day = rtclock.day();
  rtc_hour = rtclock.hour();
  rtc_mon = rtclock.month();

  log_sensor_val(&sensor1, &sensor2);
  sprintf(log_entry, "%d,%d,%d", timestamp, sensor1, sensor2);
  Serial.println(log_entry);

  if (rtc_mon != mon_time) mon_time = rtc_mon;

  if (rtc_day != day_time) {
    day_time = rtc_day;
    myFile.close();
    sd_create_folder();
  }

  if (rtc_hour != hour_time) {
    hour_time = rtc_hour;
    myFile.close();
    sd_create_file();
  }

  if (myFile) myFile.println(log_entry);
}

void log_sensor_val(uint16_t *sensor1, uint16_t *sensor2){
  sensor1_val = analogRead(PA0);
  delay(50);  // delay in between reads for stability
  sensor2_val = analogRead(PA1);

  *sensor1 = sensor1_val;
  *sensor2 = sensor2_val;
}

int exec_command(char *from_data_filtered, int from_data_action_command)
{
    uint8_t exec_fn_status;
    uint8_t function_table_index; 																		// variable fpr indexing logger_cmd_tbl_s
    uint16_t function_table_len = sizeof(logger_cmd_tbl_s)/sizeof(struct scp_cmd_tbl_s);	// size of logger_cmd_tbl_s

    // loop inside the jump table to check the command
    for (function_table_index = 0; function_table_index < function_table_len; function_table_index++)
    {
        if ((*(from_data_filtered)) == logger_cmd_tbl_s[function_table_index].command_code) break;
    }

    if (function_table_index >= function_table_len) return INVALID_CMD;

    if (from_data_action_command == ACTION_COMMAND_SET)
    {
    	// return error if setter function is not found
    	if (!logger_cmd_tbl_s[function_table_index].setter_fn) return INVALID_CMD;
      // if found
      exec_fn_status = logger_cmd_tbl_s[function_table_index].setter_fn((uint8_t *)from_data_filtered+2);

        return exec_fn_status;
    }

    else if (from_data_action_command == ACTION_COMMAND_GET)
    {
    	// verify the getter function is found
    	if (!logger_cmd_tbl_s[function_table_index].getter_fn) return INVALID_CMD;
		  // if found
    	exec_fn_status = logger_cmd_tbl_s[function_table_index].getter_fn(NULL);

		return exec_fn_status;
    }

    return INVALID_CMD;
}

void setup() {
  // Open serial communications and wait for port to open:
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PA2, OUTPUT);
  pinMode(PA3, OUTPUT);
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  sd_card_init();
  memset(msg, 0x0, MAX_MESSAGE_LENGTH);

  day_time = rtclock.day();
  hour_time = rtclock.hour();
  mon_time = rtclock.month();

  sd_create_folder();

  sd_create_file();

  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(PA2, LOW);
  digitalWrite(PA3, LOW);
}

void loop(void) {

  wait_uart_received();

  if (uart_received){
    data_received = buffer_read(msg);
    Serial.println(data_received);
    status = check_action_command(data_received);
    status = exec_command(data_received, status);
    if (status == INVALID_CMD)
		{
			Serial.println("ERR");
		}
		else if (status == VALID_CMD)
		{
			Serial.println("OK!");
		}
    memset(msg, 0x0, MAX_MESSAGE_LENGTH);
    uart_received = 0;
  }

  if (log_started){
    digitalWrite(LED_BUILTIN, HIGH);
    log_sensor_start();
    delay(1900);
    // digitalWrite(LED_BUILTIN, LOW);
    // delay(50);
    // digitalWrite(LED_BUILTIN, HIGH);
  }

  if (log_stopped) {
    myFile.close();
    digitalWrite(LED_BUILTIN, HIGH);
    log_started = 0;
    log_stopped = 0;
  }
}
