#ifndef TEMPERATURE__
#define TEMPERATURE__

struct TemperatureSenzorData {
  byte addr[8];
  float temperature_celsius;
};

typedef struct TemperatureSenzorData TemperatureSenzorData;


float raw_data_to_celsius( byte data[12], byte type_s ) {
  float celsius;

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;

  return celsius;
}

void temp_to_str( float celsius, char *buff ) {
  //char buff2[15] = "Temp: ";

  dtostrf( celsius, 5, 2, &buff[strlen(buff)] );

  //sprintf( buff2, "Temp: %4.2f", celsius );

  // graphic commands to redraw the complete screen should be placed here
  //u8g.setFont(u8g_font_7x13);
  //u8g.setFont(u8g_font_unifont);
  //u8g.setFont(u8g_font_osb21);
  
  //u8g.drawStr( 0, 9, buff2 );
  //put_string_on_display( 0, buff2 );
  //put_string_on_display( 1, buff2 );
  
  //u8g.drawStr( 0, 9, "Temp: ");
  //u8g.drawStr( 0, 9 + 11, buff );
  //u8g.drawStr( 0, 9 + 11 + 11, buff2 );
}


//void aaaaaa( TemperatureSenzorData a ) {
  
//}

TemperatureSenzorData process_temperature_sensor() {
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius;

  if ( !ds.search(addr)) {
#ifdef DEBUG_TEMPERATURE
    Serial.println("No more addresses.");
    Serial.println();
#endif
    ds.reset_search();
    delay(250);
    return;
  }

#ifdef DEBUG_TEMPERATURE
  Serial.print("ROM =");
  for ( i = 0; i < 8; i++) {
    Serial.write(' ');
    Serial.print(addr[i], HEX);
  }
#endif

  if (OneWire::crc8(addr, 7) != addr[7]) {
#ifdef DEBUG_TEMPERATURE
    Serial.println("CRC is not valid!");
#endif
    return;
  }

#ifdef DEBUG_TEMPERATURE
  Serial.println();
#endif

  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
#ifdef DEBUG_TEMPERATURE
      Serial.println("  Chip = DS18S20");  // or old DS1820
#endif
      type_s = 1;
      break;
    case 0x28:
#ifdef DEBUG_TEMPERATURE
      Serial.println("  Chip = DS18B20");
#endif
      type_s = 0;
      break;
    case 0x22:
#ifdef DEBUG_TEMPERATURE
      Serial.println("  Chip = DS1822");
#endif
      type_s = 0;
      break;
    default:
#ifdef DEBUG_TEMPERATURE
      Serial.println("Device is not a DS18x20 family device.");
#endif
      return;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end

  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.

  present = ds.reset();
  ds.select(addr);
  ds.write(0xBE);         // Read Scratchpad

#ifdef DEBUG_TEMPERATURE
  Serial.print("  Data = ");
  Serial.print(present, HEX);
  Serial.print(" ");
#endif

  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
#ifdef DEBUG_TEMPERATURE
    Serial.print(data[i], HEX);
    Serial.print(" ");
#endif
  }

#ifdef DEBUG_TEMPERATURE
  Serial.print(" CRC=");
  Serial.print(OneWire::crc8(data, 8), HEX);
  Serial.println();
#endif

  celsius = raw_data_to_celsius( data, type_s );

#ifdef DEBUG_TEMPERATURE
  Serial.print("  Temperature = ");
  Serial.print(celsius);
  Serial.println(" Celsius");
#endif

#ifdef DEBUG_TEMPERATURE
  Serial.print("Temperature:");
  
  for ( i = 0; i < 8; i++) {
    Serial.print(addr[i], HEX);
  }

  Serial.print( ":" );
  Serial.println( celsius );
#endif

  TemperatureSenzorData tsd;

  tsd.temperature_celsius = celsius;
  for ( i = 0; i < 8; i++ ) {
    tsd.addr[ i ] = addr[ i ];
  }

  return tsd;
}

#endif
