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
  This file is part of the EepromWearLevel library for Arduino.
  Definition and code are in the header file in order to use templates.

  The following options are available:
  !! does not work with templates !!
  - #define LIBCALL_EEPROM_WEAR_LEVEL: This h file can only be included once within a project as it also contains the implementation.
    To use it in multiple files, define LIBCALL_DEEP_SLEEP_SCHEDULER before all include statements except one.
  All following options are to be set before the include where no LIBCALL_DEEP_SLEEP_SCHEDULER is defined.
*/

#ifndef EEPROM_WEAR_LEVEL_H
#define EEPROM_WEAR_LEVEL_H

// -------------------------------------------------------------------------------------------------
// Definition (usually in H file)
// -------------------------------------------------------------------------------------------------
#include <EEPROM.h>

//#define NO_EEPROM_WRITES
#define EEPROM_SIZE 34

#define INDEX_VERSION 0
#define NO_DATA -1

class EepromWearLevel: EEPROMClass {
  public:
    EepromWearLevel() {
#ifdef NO_EEPROM_WRITES
      for (int i = 0; i < EEPROM_SIZE; i++) {
        eeprom[i] = 0xFF;
      }
#endif
    }

    void begin(const byte version, const int amountOfIndexes);
    void begin(const byte version, const int amountOfIndexes, const int eepromLengthToUse);
    void begin(const byte version, const int lengths[]);

    int getMaxDataLength(const int idx);
    unsigned int length();

    uint8_t read(const int idx);
    void update(const int idx, const uint8_t val);
    void write(const int idx, const uint8_t val);
    template< typename T > T &get(const int idx, T &t);
    template< typename T > const T &put(const int idx, const T &t);

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
    byte eeprom[EEPROM_SIZE];
#endif
    unsigned int amountOfIndexes;

    void init(const byte version);
    template< typename T > const T &put(const int idx, const T &t, const bool update);
    const inline int getControlBytesCount(const int index);
    const int findIndex(const EepromConfig &config, const int controlBytesCount);
    const int findControlByteIndex(const int startIndex, const int length);
    const inline byte readByte(const int index);

    void programBitToZero(int index, byte bitIndex);
    void programZeroBitsToZero(int index, byte byteWithZeros);
    void clearBytesToOnes(int startIndex, int length);
    void clearByteToOnes(int index);
    void printBinWithLeadingZeros(byte value);
};


/**
   the instance of EepromWearLevel
*/
extern EepromWearLevel eepromWearLevel;

#ifndef LIBCALL_EEPROM_WEAR_LEVEL
// -------------------------------------------------------------------------------------------------
// Implementation (usuallly in CPP file)
// -------------------------------------------------------------------------------------------------
static EepromWearLevel eepromWearLevel;

void EepromWearLevel::begin(const byte version, const int amountOfIndexes) {
  begin(version, amountOfIndexes, EEPROMClass::length());
}

void EepromWearLevel::begin(const byte version, const int amountOfIndexes, const int eepromLengthToUse) {
  int startIndex = 1; // index 0 reserved for the version
  EepromWearLevel::amountOfIndexes = amountOfIndexes;
  // +1 to store a place holder element in the last
  // place to get the lenth of the last element
  eepromConfig = new EepromConfig[amountOfIndexes + 1];
  const int singleLength = eepromLengthToUse / amountOfIndexes;
  int index;
  for (index = 0; index < amountOfIndexes; index++) {
    eepromConfig[index].startIndexControlBytes = startIndex;
    startIndex += singleLength;
  }
  // the last one as a placeholder to calculate the length of the last real element
  eepromConfig[index].startIndexControlBytes = startIndex;

  init(version);
}

void EepromWearLevel::begin(const byte version, const int lengths[]) {
  int startIndex = 1; // index 0 reserved for the version
  amountOfIndexes = sizeof(lengths);
  // +1 to store a place holder element in the last
  // place to get the lenth of the last element
  eepromConfig = new EepromConfig[amountOfIndexes + 1];
  int index;
  for (index = 0; index < sizeof(lengths); index++) {
    eepromConfig[index].startIndexControlBytes = startIndex;
    startIndex += lengths[index];
  }
  // the last one as a placeholder to calculate the length of the last real element
  eepromConfig[index].startIndexControlBytes = startIndex;

  init(version);
}

void EepromWearLevel::init(const byte version) {
  const byte previousVersion = readByte(INDEX_VERSION);
#ifndef NO_EEPROM_WRITES
  EEPROMClass::write(INDEX_VERSION, version);
#else
  eeprom[0] = version;
#endif

  // -1 because the last one is a placeholder
  for (int index = 0; index < amountOfIndexes; index++) {
    const int controlBytesCount = getControlBytesCount(index);
    if (version != previousVersion) {
      clearBytesToOnes(eepromConfig[index].startIndexControlBytes, controlBytesCount);
    }

    eepromConfig[index].lastIndexRead = findIndex(eepromConfig[index], controlBytesCount);
  }
}

int EepromWearLevel::getMaxDataLength(const int idx) {
  return eepromConfig[idx + 1].startIndexControlBytes
         - eepromConfig[idx].startIndexControlBytes
         - getControlBytesCount(idx);
}

unsigned int EepromWearLevel::length() {
  return amountOfIndexes;
}

uint8_t EepromWearLevel::read(const int idx) {
  uint8_t value;
  return get(idx, value);
}

void EepromWearLevel::update(const int idx, const uint8_t val) {
  put(idx, val, true);
}

void EepromWearLevel::write(const int idx, const uint8_t val) {
  put(idx, val, false);
}

template< typename T > T &EepromWearLevel::get(const int idx, T &t) {
  const EepromConfig &config = eepromConfig[idx];

  const int lastIndex = config.lastIndexRead;
  if (lastIndex != NO_DATA) {
    const int dataLength = sizeof(t);
    // +1 because it is the last index
    const int firstIndex = lastIndex + 1 - dataLength;
#ifndef NO_EEPROM_WRITES
    return EEPROMClass::get(firstIndex, t);
#else
    byte *values = (byte*) &t;
    for (int i = 0; i < dataLength; i++) {
      values[i] = eeprom[firstIndex + i];
    }
#endif
  } else {
    Serial.println(F("no data"));
  }
  return t;
}

template< typename T > const T &EepromWearLevel::put(const int idx, const T &t) {
  put(idx, t, true);
}

template< typename T > const T &EepromWearLevel::put(const int idx, const T &t, const bool update) {
  const int dataLength = sizeof(t);
  Serial.print(F("dataLength: "));
  Serial.println(dataLength);
  if (dataLength > getMaxDataLength(idx)) {
    Serial.print(F("dataLength too long. Max: "));
    Serial.print(getMaxDataLength(idx));
    Serial.print(F(", is: "));
    Serial.println(dataLength);
    return t;
  }

  EepromConfig &config = eepromConfig[idx];
  const int controlBytesCount = getControlBytesCount(idx);
  int previousLastIndex = config.lastIndexRead;
  if (previousLastIndex == NO_DATA) {
    // no data yet so we set the index to one before the first index
    previousLastIndex = config.startIndexControlBytes + controlBytesCount - 1;
  }
  const byte *values = (const byte*) &t;
  if (update) {
    boolean equal = true;
    const int previousStartIndex = previousLastIndex - (dataLength - 1);
    for (int i = 0; previousStartIndex + i <= previousLastIndex; i++) {
      if (readByte(previousStartIndex + i) != values[i]) {
        equal = false;
        break;
      }
    }
    if (equal) {
      Serial.println(F("================================ equal"));
      return t;
    }
  }

  int newStartIndex = previousLastIndex + 1;
  // eepromConfig[idx + 1].startIndexControlBytes is the first
  // index of the next one. The last one has a placehoder for
  // this purpose.
  // not >= because newAbsoluteIndex is the first write position
  if (newStartIndex + dataLength > eepromConfig[idx + 1].startIndexControlBytes) {
    // last bit is not set
    // all used, start again
    Serial.println(F("-------------------------------- ALL USED"));
    clearBytesToOnes(config.startIndexControlBytes, controlBytesCount);
    newStartIndex = config.startIndexControlBytes + controlBytesCount;
  }

#ifndef NO_EEPROM_WRITES
  const T &resT = EEPROMClass::put(newStartIndex, t);
#else
  const T &resT = t;
  for (int i = 0; i < dataLength; i++) {
    eeprom[newStartIndex + i] = values[i];
  }
#endif

  // -1 because it is the last index
  config.lastIndexRead = newStartIndex + dataLength - 1;
  Serial.print(F("lastIndex: "));
  Serial.println(config.lastIndexRead);
  const int startIndexData = config.startIndexControlBytes + controlBytesCount;
  const int startIndexRelative = newStartIndex - startIndexData;
  int controlByteIndex = startIndexRelative / 8;
  byte newBitPosInControlByte = startIndexRelative - controlByteIndex * 8;

  // unset
  byte writeMask = 0xFF;
  for (int i = 0; i < dataLength; i++) {
    writeMask  &= ~(1 << (7 - newBitPosInControlByte));
    newBitPosInControlByte++;
    if (newBitPosInControlByte > 7) {
      // write and go to next byte
      Serial.print(F("write in loop "));
      Serial.print(i);
      Serial.print(F(" "));
      printBinWithLeadingZeros(writeMask);
      Serial.println();
      programZeroBitsToZero(config.startIndexControlBytes + controlByteIndex, writeMask);
      writeMask = 0xFF;
      newBitPosInControlByte = 0;
      controlByteIndex++;
    }
  }
  if (writeMask != 0xFF) {
    programZeroBitsToZero(config.startIndexControlBytes + controlByteIndex, writeMask);
  }
  Serial.print(F("after loop "));
  Serial.print(F(" "));
  printBinWithLeadingZeros(writeMask);
  Serial.println();
  return resT;
}

void EepromWearLevel::printBinary(int startIndex, int endIndex) {
  for (int i = startIndex; i <= endIndex; i++) {
    const byte value = readByte(i);
    Serial.print(i);
    Serial.print(F(":"));
    printBinWithLeadingZeros(value);
    Serial.print(F("/"));
    Serial.print(value);
    Serial.print(F(" "));
    if ((i+1) % 10 == 0) {
      Serial.println();
    }
  }
  Serial.println();
}

void EepromWearLevel::printStatus() {
  Serial.println(F("EepromWearLevel status: "));
  for (int index = 0; index < amountOfIndexes; index++) {
    const int controlBytesCount = getControlBytesCount(index);
    Serial.print(index);
    Serial.print(F(": "));
    Serial.print(eepromConfig[index].startIndexControlBytes);
    Serial.print(F("-"));
    Serial.print(eepromConfig[index + 1].startIndexControlBytes);
    Serial.print(F(" with "));
    Serial.print(controlBytesCount);
    Serial.print(F(" ctrl bytes at "));
    Serial.println(eepromConfig[index].lastIndexRead);
  }
}

const inline int EepromWearLevel::getControlBytesCount(const int idx) {
  const int length = eepromConfig[idx + 1].startIndexControlBytes - eepromConfig[idx].startIndexControlBytes;
  if ((length % 9) == 0) {
    // div exact
    return length / 9;
  } else {
    // div not exact so we need to round up
    return length / 9 + 1;
  }
}

const int EepromWearLevel::findIndex(const EepromConfig &config, const int controlBytesCount) {
  const int startIndexData = config.startIndexControlBytes + controlBytesCount;
  const int controlByteIndex = findControlByteIndex(config.startIndexControlBytes, controlBytesCount);
  const byte currentByte = readByte(controlByteIndex);

  byte mask = 1;
  int bitPosInByte = 7;
  // do while bit set and bit still in mask
  while ((currentByte & mask) != 0 && mask != 0) {
    mask <<= 1;
    bitPosInByte--;
  }

  const int controlByteIndexRelative = controlByteIndex - config.startIndexControlBytes;
  Serial.print(config.startIndexControlBytes);
  Serial.print(F(": ctrl byte/bits: "));
  Serial.print(controlByteIndexRelative);
  Serial.print(F("/"));
  Serial.print(bitPosInByte);
  const int amountOfWholeBytes = controlByteIndexRelative;
  const int indexRelative = amountOfWholeBytes * 8 + bitPosInByte;
  Serial.print(F(" "));
  Serial.println(startIndexData + indexRelative);
  if (indexRelative == -1) {
    // not data yet
    return NO_DATA;
  } else {
    return startIndexData + indexRelative;
  }
}

/**
   returns the index of the control byte that contains the bit which
   points to the next write position.
   The index is inside of control bytes even if all used.
*/
const int EepromWearLevel::findControlByteIndex(const int startIndex, const int length) {
  const int endIndex = startIndex + length - 1;
  int lowerBound = startIndex;
  int upperBound = endIndex;
  int midPoint = -1;
  int comparisons = 0;
  int index = -1;

  byte currentByte = 0xFF;
  while (lowerBound <= upperBound) {
    comparisons++;

    // compute the mid point
    midPoint = lowerBound + (upperBound - lowerBound) / 2;

    // data found
    currentByte = readByte(midPoint);
    if (currentByte != 0 && currentByte != 0xFF) {
      Serial.print(F("Total comparisons made: "));
      Serial.println(comparisons);
      return midPoint;
    } else {
      if (currentByte == 0) {
        // data is in upper half
        lowerBound = midPoint + 1;
      } else {
        // data is in lower half
        upperBound = midPoint - 1;
      }
    }
  }

  Serial.print(F("Total comparisons made: "));
  Serial.println(comparisons);

  if (currentByte == 0) {
    int controlByteIndex = midPoint;
    while (currentByte == 0 && controlByteIndex < endIndex) {
      controlByteIndex++;
      currentByte = readByte(controlByteIndex);
    }
    return controlByteIndex;
  } else {
    int controlByteIndex = midPoint;
    while (currentByte == 0xFF && controlByteIndex >= startIndex) {
      controlByteIndex--;
      currentByte = readByte(controlByteIndex);
    }
    // we want the byte where all bits are still set
    // except when it is the last one.
    if (controlByteIndex >= endIndex) {
      // do not go to next because it is the last one
      return controlByteIndex;
    } else {
      // go to next as current is 0
      return controlByteIndex + 1;
    }
  }
}

const inline byte EepromWearLevel::readByte(const int index) {
#ifndef NO_EEPROM_WRITES
  return EEPROMClass::read(index);
#else
  return eeprom[index];
#endif
}


// http://www.tronix.io/data/avratmega/eeprom/
void EepromWearLevel::programBitToZero(int index, byte bitIndex) {
  byte value = 0xFF;
  value &= ~(1 << (7 - bitIndex));
  Serial.print(F("writeBite: "));
  printBinWithLeadingZeros(value);
  Serial.println();
  programZeroBitsToZero(index, value);
}

#ifdef NO_EEPROM_WRITES
// emulate EEPROM behaviour to program only bits that are 0
void EepromWearLevel::programZeroBitsToZero(int index, byte byteWithZeros) {
  byte mask = 1;
  int bitPosInByte = 7;
  // do while bit still in mask
  while (mask != 0) {
    if ((byteWithZeros & mask) == 0) {
      eeprom[index] &= ~(1 << (7 - bitPosInByte));
      Serial.println(bitPosInByte);
    }
    mask <<= 1;
    bitPosInByte--;
  }
}
#else
void EepromWearLevel::programZeroBitsToZero(int index, byte byteWithZeros) {
  // EEPROM Mode Bits.
  // EEPM1.0 = 0 0 - Mode 0 Erase & Write in one operation.
  // EEPM1.0 = 0 1 - Mode 1 Erase only.
  // EEPM1.0 = 1 0 - Mode 2 Write only.
  EECR |= 1 << EEPM1;
  EECR &= ~(1 << EEPM0);
  // EEPROM Ready Interrupt Enable.
  // EERIE = 0 - Interrupt Disable.
  // EERIE = 1 - Interrupt Enable.
  EECR &= ~(1 << EERIE);

  // Set EEPROM address - 0x000 - 0x3FF.
  EEAR = index;
  // Data write into EEPROM.
  EEDR = byteWithZeros;

  uint8_t u8SREG = SREG;
  cli();
  // Wait for completion previous write.
  while (EECR & (1 << EEPE));
  // EEMPE = 1 - Master Write Enable.
  EECR |= (1 << EEMPE);
  // EEPE = 1 - Write Enable.
  EECR  |=  (1 << EEPE);
  SREG = u8SREG;
}
#endif

void EepromWearLevel::clearBytesToOnes(int fromIndex, int length) {
  for (int i = fromIndex; i < fromIndex + length; i++) {
    if (readByte(i) != 0xFF) {
#ifndef NO_EEPROM_WRITES
      clearByteToOnes(i);
#else
      eeprom[i] = 0xFF;
#endif
      Serial.print(F("clear byte: "));
      Serial.println(i);
    }
  }
}

void EepromWearLevel::clearByteToOnes(int index) {
  // EEPROM Mode Bits.
  // EEPM1.0 = 0 0 - Mode 0 Erase & Write in one operation.
  // EEPM1.0 = 0 1 - Mode 1 Erase only.
  // EEPM1.0 = 1 0 - Mode 2 Write only.
  // set EEPM0
  EECR |= 1 << EEPM0;
  // clear EEPM1
  EECR &= ~(1 << EEPM1);
  // EEPROM Ready Interrupt Enable.
  // EERIE = 0 - Interrupt Disable.
  // EERIE = 1 - Interrupt Enable.
  EECR &= ~(1 << EERIE);

  // Set EEPROM address - 0x000 - 0x3FF.
  EEAR = index;//0x000;

  uint8_t u8SREG = SREG;
  cli();
  // Wait for completion previous write.
  while (EECR & (1 << EEPE));
  // EEMPE = 1 - Master Write Enable.
  EECR |= (1 << EEMPE);
  // EEPE = 1 - Write Enable.
  EECR  |=  (1 << EEPE);
  SREG = u8SREG;
}

void EepromWearLevel::printBinWithLeadingZeros(byte value) {
  byte mask = 1 << 7;
  for (int bitPosInByte = 0; bitPosInByte < 8; bitPosInByte++) {
    Serial.print((value & mask) != 0 ? 1 : 0);
    mask >>= 1;
  }
}

#endif // #ifndef LIBCALL_EEPROM_WEAR_LEVEL
#endif // #ifndef EEPROM_WEAR_LEVEL_H

