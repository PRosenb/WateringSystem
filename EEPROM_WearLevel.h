/*
    Copyright 2016-2016 Peter Rosenberg

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/
/*
  This file is part of the EEPROM_WearLevel library for Arduino.
*/

#ifndef EEPROM_WEAR_LEVEL_H
#define EEPROM_WEAR_LEVEL_H

#include <EEPROM.h>

//#define NO_EEPROM_WRITES
#define FAKE_EEPROM_SIZE 34

#define INDEX_VERSION 0
#define NO_DATA -1

class EEPROM_WearLevel: EEPROMClass {
  public:
    EEPROM_WearLevel();

    void begin(const byte version, const int amountOfIndexes);
    void begin(const byte version, const int amountOfIndexes, const int eepromLengthToUse);
    void begin(const byte version, const int lengths[]);

    int getMaxDataLength(const int idx);
    unsigned int length();

    uint8_t read(const int idx);
    void update(const int idx, const uint8_t val);
    void write(const int idx, const uint8_t val);

    template< typename T > T &get(const int idx, T &t) {
      return getImpl(idx, t);
    }
    template< typename T > const T &put(const int idx, const T &t) {
      put(idx, t, true);
    }

    void printStatus();
    void printBinary(int startIndex, int endIndex);

  private:
    class EepromConfig {
      public:
        int startIndexControlBytes;
        // NO_DATA (-1) for no data
        int lastIndexRead;
    };

    EepromConfig *eepromConfig;
#ifdef NO_EEPROM_WRITES
    byte fakeEeprom[FAKE_EEPROM_SIZE];
#endif
    unsigned int amountOfIndexes;

    void init(const byte version);

    int getWriteStartIndex(const int idx, const int dataLength, const byte *values,
                           const bool update, const int controlBytesCount);
    void updateControlBytes(int idx, int newStartIndex, int dataLength, const int controlBytesCount);

    const int getControlBytesCount(const int index);
    const int findIndex(const EepromConfig &config, const int controlBytesCount);
    const int findControlByteIndex(const int startIndex, const int length);
    const inline byte readByte(const int index);

    void programBitToZero(int index, byte bitIndex);
    void programZeroBitsToZero(int index, byte byteWithZeros, int retryCount);
    void programZeroBitsToZero(int index, byte byteWithZeros);
    void clearBytesToOnes(int startIndex, int length);
    void clearByteToOnes(int index);
    void printBinWithLeadingZeros(byte value);

    // --------------------------------------------------------
    // implementation of template methods
    // --------------------------------------------------------
    template< typename T > T &getImpl(const int idx, T &t) {
      const int lastIndex = eepromConfig[idx].lastIndexRead;
      if (lastIndex != NO_DATA) {
        const int dataLength = sizeof(t);
        // +1 because it is the last index
        const int firstIndex = lastIndex + 1 - dataLength;
#ifndef NO_EEPROM_WRITES
        return EEPROMClass::get(firstIndex, t);
#else
        byte *values = (byte*) &t;
        for (int i = 0; i < dataLength; i++) {
          values[i] = fakeEeprom[firstIndex + i];
        }
#endif
      } else {
        Serial.println(F("no data"));
      }
      return t;
    }

    template< typename T > const T &put(const int idx, const T &t, const bool update) {
      const int dataLength = sizeof(t);
      const byte *values = (const byte*) &t;
      const int controlBytesCount = getControlBytesCount(idx);

      const int writeStartIndex = getWriteStartIndex(idx, dataLength, values, update, controlBytesCount);
      if (writeStartIndex < 0) {
        return t;
      }
#ifndef NO_EEPROM_WRITES
      const T &resT = EEPROMClass::put(writeStartIndex, t);
#else
      const T &resT = t;
      for (int i = 0; i < dataLength; i++) {
        fakeEeprom[writeStartIndex + i] = values[i];
      }
#endif
      updateControlBytes(idx, writeStartIndex, dataLength, controlBytesCount);
      return t;
    }
};


/**
   the instance of EEPROM_WearLevel
*/
extern EEPROM_WearLevel EEPROMwl;

#endif // #ifndef EEPROM_WEAR_LEVEL_H

