/*
   Used PINs:

    2 - RF receiver data
    3 - RF receiver data
    4 - RF transmitter data
    6 - RF transmitter Vcc
    9 - Temperature sensor

   10 - Ethernet Shield
   11 - Ethernet Shield
   12 - Ethernet Shield
   13 - Ethernet Shield
   
   A4 - Display SDA
   A5 - Display SCK
*/

// setup u8g object, please remove comment from one of the following constructor calls
// IMPORTANT NOTE: The following list is incomplete. The complete list of supported
// devices with all constructor calls is here: https://github.com/olikraus/u8glib/wiki/device

#include "U8glib.h"
U8GLIB_SSD1306_128X32 u8g(U8G_I2C_OPT_NONE);  // I2C / TWI

#include <OneWire.h> // for Dallas temperature sensor

#include <RemoteTransmitter.h>  // ActionTransmitter to control socket plugs

#include <RCSwitch.h>
#include <SensorReceiver.h> // weather station 

#include <SPI.h>         // needed for Arduino versions later than 0018
#include <Ethernet.h>
#include <EthernetUdp.h>         // UDP library from: bjoern@cs.stanford.edu 12/30/2008

#include "ring_buffer.h"

// OneWire DS18S20, DS18B20, DS1822 Temperature Example
//
// http://www.pjrc.com/teensy/td_libs_OneWire.html
//
// The DallasTemperature library can do all this work for you!
// http://milesburton.com/Dallas_Temperature_Control_Library


// defines
#define LIGHT_SWITCH_DATAGRAM                0
#define LIGHT_SWITCH_DATAGRAM_REPEATS_IDX    1
#define LIGHT_SWITCH_DATAGRAM_SOCKET_ID_IDX  2
#define LIGHT_SWITCH_DATAGRAM_COMMAND_IDX    3

#define DISPLAY_LEN     20
#define TEMPERATURE_LEN 5

#define METEO_RECEIVE_PIN       2
#define RC_RECEIVE_PIN          3
#define RC_TRANSMIT_PIN         4
#define DALLAS_TEMPERATURE_PIN  9

#define SENSOR_TEMPERATURE_METEOSTATION  1
#define SENSOR_TEMPERATURE_RADIATOR      2

#define nDEBUG_TEMPERATURE
#define nDEBUG_RF
#define DEBUG_ETHERNET

RCSwitch rc_receiver = RCSwitch();

OneWire  ds( DALLAS_TEMPERATURE_PIN );  // on pin 9 (a 4.7K resistor is necessary)


#include "temperature.h"


ActionTransmitter actionTransmitter( RC_TRANSMIT_PIN );

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xE1
};

IPAddress ip( 192, 168, 2, 32 );
IPAddress b_ip( 255, 255, 255, 255 );
IPAddress raspi_ip( 192, 168, 2, 31 );

unsigned int b_port = 10000;
unsigned int local_port = 8888;      // local port to listen on
unsigned int raspi_port = 9999;

// buffers for receiving and sending data
char packet_buffer[UDP_TX_PACKET_MAX_SIZE];  //buffer to hold incoming packet,
char reply_buffer[] = "ACK";       // a string to send back

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

int loop_counter = 0;

const int info_led_pin = 13;
const int rf_transmitter_vcc_pin = 6;

const char compile_date[] = __DATE__ " " __TIME__;

const byte LINE_Y_IDX[ 2 ] = { 9, 30 };

char display_line_2_buff[ DISPLAY_LEN ] = "";
char display_text_default[ DISPLAY_LEN ] = "Meteo: ";
char display_text_received[ DISPLAY_LEN ] = "";

float last_outside_temperature = 0.0;
bool last_outside_temperature_set = false;

RingBuffer *rb_meteostation_temperatures = ring_buffer_new();
RingBuffer *rb_radiator_temperatures = ring_buffer_new();


void setup()
{
  Serial.begin(115200);
  pinMode( info_led_pin, OUTPUT );

  rc_receiver.enableReceive( digitalPinToInterrupt( RC_RECEIVE_PIN ) ); // Receiver on interrupt 1 => that is pin #3
  SensorReceiver::init( digitalPinToInterrupt( METEO_RECEIVE_PIN ), process_rf_temp_humi );  // Receiver on interrupt 0 => that is pin #2

  digitalWrite( rf_transmitter_vcc_pin, LOW );

  // assign default color value
  if ( u8g.getMode() == U8G_MODE_R3G3B2 ) {
    u8g.setColorIndex(255);     // white
  }
  else if ( u8g.getMode() == U8G_MODE_GRAY2BIT ) {
    u8g.setColorIndex(3);         // max intensity
  }
  else if ( u8g.getMode() == U8G_MODE_BW ) {
    u8g.setColorIndex(1);         // pixel on
  }
  else if ( u8g.getMode() == U8G_MODE_HICOLOR ) {
    u8g.setHiColorByRGB(255, 255, 255);
  }

  Ethernet.begin( mac, ip );
  Udp.begin( local_port );
} // setup


void process_rf_temp_humi(byte *data) {
  //Serial.println( "In process_rf_temp_humi" );
  // is data a ThermoHygro-device?
  if ((data[3] & 0x1f) == 0x1e) {
    // Yes!

    byte channel, randomId;
    int temp;
    byte humidity;

    // Decode the data
    SensorReceiver::decodeThermoHygro(data, channel, randomId, temp, humidity);

    // Print humidity
    //Serial.print(" deg, Humidity: ");
    //Serial.print(humidity);
    //Serial.print("% REL");

    // Print channel
    //Serial.print(", Channel: ");
    //Serial.println(channel, DEC); 

    last_outside_temperature = temp / 10.0;
    last_outside_temperature_set = true;

    float actual_temp = temp / 10.0f;
    ring_buffer_add( rb_meteostation_temperatures, actual_temp );

    //Serial.print( "RB Temperatures of meteostation: " );
    //ring_buffer_print( rb_meteostation_temperatures );

    //Serial.print( "Average meteostation temp: " );
    //bool computed;
    //float meteostation_avg_temp = ring_buffer_average_value( rb_meteostation_temperatures );
    //Serial.println( meteostation_avg_temp );

    /*
    Serial.print( "RB Temperatures of meteostation: " );
    for ( int i = 0; i < rb_meteostation_temperatures->numElements( rb_meteostation_temperatures ); i++ ) {
      float *temp = (float*)rb_meteostation_temperatures->peek( rb_meteostation_temperatures, i );
      Serial.print( *temp );
      Serial.print( " " );
    }
    Serial.println( " " );
    Serial.print( "Average meteostation temp: " );
    bool computed;
    float meteostation_avg_temp = average_temperature_in_ring_buffer( rb_meteostation_temperatures, & computed );
    Serial.println( meteostation_avg_temp );
    */

    Serial.print( "Meteostation temperature (current): " );
    Serial.println( last_outside_temperature );
  }
} // process_rf_temp_humi


void send_rf_socket_signal( const char *socket_id, const int repeats, const int switch_on ) {

#ifdef DEBUG_RF
  Serial.print( "Socket operation: " );
  Serial.print( " socket_id= " );
  Serial.print( socket_id[ 0 ] );
  Serial.print( " repeats= " );
  Serial.print( repeats );
  Serial.print( " switch_on= " );
  Serial.println( switch_on );

  Serial.print( "Sending RF signal" );
#endif

  cli();
  for ( char i = 0; i < repeats; i++ ) {
    // Switch on Action-device
    digitalWrite( rf_transmitter_vcc_pin, HIGH );
    actionTransmitter.sendSignal( 31, socket_id[ 0 ], switch_on );
    digitalWrite( rf_transmitter_vcc_pin, LOW );

    if ( switch_on ) {
      digitalWrite( info_led_pin, HIGH );
    }
    else {
      digitalWrite( info_led_pin, LOW );
    }

#ifdef DEBUG_RF
    Serial.print( " . " );
#endif

    delay( 50 );
  }
  sei();

#ifdef DEBUG_RF
    Serial.println( "" );
#endif
} // send_rf_socket_signal


void handle_ethernet_rf_packet( char *datagram ) {
  char repeats_str[ 3 ];

  //Serial.print( "Datagram: " );
  //Serial.print( datagram );
  
  char socket_id = datagram[ 3 ];
  int switch_on = datagram[ 8 ] - '0';


  strncpy( repeats_str, datagram+5, 2 );
  repeats_str[ 2 ] = '\0';

  int repeats = atoi( repeats_str );

  /*
  Serial.print( "socket= " );
  Serial.print( socket_id );
  Serial.print( " repeats= " );
  Serial.print( repeats );
  Serial.print( " ( " );
  Serial.print( repeats_str );
  Serial.print( " )");
  Serial.print( " switch_on= " );
  Serial.print( switch_on );

  Serial.print( " ( " );
  Serial.print( datagram[ 8 ] );
  Serial.println( " )");
  */

  send_rf_socket_signal( &socket_id, repeats, switch_on );
} // handle_ethernet_rf_packet


void put_string_on_display( byte line_no, char *str ) {
  u8g.setFont( u8g_font_7x13 );
  u8g.drawStr( 0, LINE_Y_IDX[ line_no ], str );
} // put_string_on_display


// TODO: Add check for str_len in idx
void generate_temperature_udp_response( char * str, const int str_len, const byte * sensor_addr, const int temperature_sensor, const float temperature, const byte avg_computed ) {
  int idx = 0;

  if ( temperature_sensor == SENSOR_TEMPERATURE_METEOSTATION ) {
    sprintf( str, "Temp:meteostation:" );
    idx += 18;
  }
  else {
    sprintf( str, "Temp:" );
    idx += 5;

    for ( byte i = 0; i < 8; i++ ) {
      sprintf( str + idx, "%02x", sensor_addr[ i ] );
      idx += 2;
    }
    sprintf( str + idx, ":" );
    idx += 1;

  }
  sprintf( str + idx, "%d:", avg_computed );
  idx += 2;
  temp_to_str( temperature, str + idx );
}


void loop() {

  /**/
  // receiving from 433 module
  if (rc_receiver.available()) {

      //Serial.print( "Calling SensorReceiver interruptHandler..." );
      //SensorReceiver::interruptHandler();

    int value = rc_receiver.getReceivedValue();

    if (value == 0) {

    } else {
#ifdef DEBUG_RF
      Serial.print("RF:");
      Serial.println( rc_receiver.getReceivedValue() );
#endif

      char s[ 20 ];
      sprintf( s, "RF:%lu\n", rc_receiver.getReceivedValue() );

      Udp.beginPacket( raspi_ip, raspi_port );
      Udp.write( s );
      Udp.endPacket();

#ifdef DEBUG_RF
      Serial.print("UDP:");
      Serial.println( s );
#endif
}

    rc_receiver.resetAvailable();
  }
  /**/

  /*
  if (Serial.available()) {
    process_incomming_serial_communication();
  }
  */

  char display_line_1_buff[ DISPLAY_LEN ] = "Radiator: ";

  TemperatureSenzorData tsd = process_temperature_sensor();
#ifdef DEBUG_TEMPERATURE
  Serial.print( "Current radiator temperature: " );
  Serial.println( tsd.temperature_celsius );
#endif
  ring_buffer_add( rb_radiator_temperatures, tsd.temperature_celsius );

#ifdef DEBUG_TEMPERATURE
  {
    Serial.print( "RB Temperatures of radiator: " );
    unsigned char last_idx = ring_buffer_last_index( rb_radiator_temperatures );
    //Serial.print( "Last index= " );
    //Serial.print( last_idx );
    //Serial.print( " Capacity= " );
    //Serial.print( RB_CAPACITY );
    //Serial.print( " Current= " );
    //Serial.print( rb_radiator_temperatures->current );

    ring_buffer_print( rb_radiator_temperatures );
    //for ( int i = 0; i < last_idx; i++ ) {
    //    float temp = rb_radiator_temperatures->rb[ i ];
    //    Serial.print( temp );
    //    Serial.print( " " );
    //}

    Serial.println();
  }
  /**/
#endif

  byte computed_meteostation = 0;
  byte computed_radiator = 0;
  
  float radiator_avg_temp = ring_buffer_average_value( rb_radiator_temperatures, &computed_radiator );
  float meteostation_avg_temp = ring_buffer_average_value( rb_meteostation_temperatures, &computed_meteostation );

#ifdef DEBUG_TEMPERATURE
  Serial.print( "Average radiator temp: " );
  Serial.print( radiator_avg_temp );
  Serial.print( ", computed= " );
  Serial.println( computed_radiator );
#endif

  // if there's data available, read a packet
  int packetSize = Udp.parsePacket();
  if ( packetSize ) {
    strcpy( packet_buffer, "" );

#ifdef DEBUG_ETHERNET
    Serial.print( "Received packet of size " );
    Serial.println( packetSize );
    //Serial.print( "From " );
    //IPAddress remote = Udp.remoteIP();
    //for ( int i = 0; i < 4; i++ ) {
    //  Serial.print( remote[ i ], DEC );
    //  if ( i < 3 ) {
    //    Serial.print( "." );
    //  }
    //}
    //Serial.print( ", port " );
    //Serial.println( Udp.remotePort() );
#endif

    // read the packet into packetBufffer
    Udp.read( packet_buffer, UDP_TX_PACKET_MAX_SIZE );

#ifdef DEBUG_ETHERNET
    Serial.println( "Contents:" );
    Serial.println( packet_buffer );
    Serial.println( "END Packet Debug Info" );
#endif

    if ( !strncmp( packet_buffer, "version", 7 ) ) {
      Udp.beginPacket( Udp.remoteIP(), Udp.remotePort() );
      Udp.write( compile_date );
      Udp.endPacket();
    }
    else if ( !strncmp( packet_buffer, "rf", 2 ) ) {
      // "rf A:10:1" switch:repeats:[0,1]
      handle_ethernet_rf_packet( packet_buffer );
      Udp.beginPacket( Udp.remoteIP(), Udp.remotePort() );
      Udp.write( "INFO: Radio packet sent" );
      Udp.endPacket();
    }
    else if ( !strncmp( packet_buffer, "temperature-meteostation", 24 ) ) {
      Udp.beginPacket( Udp.remoteIP(), Udp.remotePort() );
      char s[ 30 ];

      generate_temperature_udp_response( s, 30, NULL, SENSOR_TEMPERATURE_METEOSTATION, meteostation_avg_temp, computed_meteostation );

#ifdef DEBUG_ETHERNET
      Serial.print( "Will send UDP response: " );
      Serial.println( s );
#endif
      Udp.write( s );
      Udp.endPacket();
    }
    else if ( !strncmp( packet_buffer, "temperature", 11 ) ) {
      //cli();
      Udp.beginPacket( Udp.remoteIP(), Udp.remotePort() );
      char s[ 34 ];

      generate_temperature_udp_response( s, 34, tsd.addr, SENSOR_TEMPERATURE_RADIATOR, radiator_avg_temp, computed_radiator );

#ifdef DEBUG_ETHERNET
      Serial.print( "Will send UDP response: " );
      Serial.println( s );
#endif

      Udp.write( s );
      Udp.endPacket();
      //sei();
    }
    /*
    else if ( !strncmp( packet_buffer, "text", 4 ) ) {
      //cli();
      //strcpy( b, packet_buffer+5 );
      if ( strlen( packet_buffer+5 ) < ( DISPLAY_LEN - TEMPERATURE_LEN ) ) {
        strcpy( display_text_received, packet_buffer + 5 );
      }
      //strcpy( b, packet_buffer+5 );
      
      Udp.write( reply_buffer );
      Udp.endPacket();
      //sei();
    }

    // send a reply to the IP address and port that sent us the packet we received
    //Udp.beginPacket( Udp.remoteIP(), Udp.remotePort() );
    //Udp.write( reply_buffer );
    //Udp.endPacket();
  */
  }
  delay( 10 );

  if ( strlen( display_text_received ) ) {
    strcpy( display_line_2_buff, display_text_received );
  }
  else {
    strcpy( display_line_2_buff, display_text_default );
  }


  if ( ring_buffer_last_index( rb_radiator_temperatures ) > 0 ) {
    temp_to_str( radiator_avg_temp, display_line_1_buff );
  }
  else {
    temp_to_str( 0.0f, display_line_1_buff );
  }
  if ( ring_buffer_last_index( rb_meteostation_temperatures ) > 0 ) {
    temp_to_str( meteostation_avg_temp, display_line_2_buff );
  }
  else {
    temp_to_str( 0.0f, display_line_2_buff );
  }


  u8g.firstPage();
  do {
    put_string_on_display( 0, display_line_1_buff );
    put_string_on_display( 1, display_line_2_buff );
  } while ( u8g.nextPage() );

  loop_counter++;


  // broadcast info to LAN
  if ( loop_counter % 10 == 0 ) {
    //Serial.print(".");
    loop_counter = 0;

    Udp.beginPacket(b_ip, b_port);

    Udp.write( display_line_1_buff );
    Udp.write( "\n" );
    Udp.write( display_line_2_buff );
    Udp.write( "\n" );

    Udp.endPacket();
  }


  // rebuild the picture after some delay
  delay(500);
} // loop

