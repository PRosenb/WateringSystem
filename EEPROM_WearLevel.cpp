#include <Arduino.h>
#include "EEPROM_WearLevel.h"

static EEPROM_WearLevel EEPROMwl;

EEPROM_WearLevel::EEPROM_WearLevel() {
#ifdef NO_EEPROM_WRITES
  for (int i = 0; i < FAKE_EEPROM_SIZE; i++) {
    fakeEeprom[i] = 0xFF;
  }
#endif
}

void EEPROM_WearLevel::begin(const byte version, const int amountOfIndexes) {
  begin(version, amountOfIndexes, EEPROMClass::length());
}

void EEPROM_WearLevel::begin(const byte version, const int amountOfIndexes, const int eepromLengthToUse) {
  int startIndex = 1; // index 0 reserved for the version
  EEPROM_WearLevel::amountOfIndexes = amountOfIndexes;
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

void EEPROM_WearLevel::begin(const byte version, const int lengths[]) {
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

void EEPROM_WearLevel::init(const byte version) {
  const byte previousVersion = readByte(INDEX_VERSION);
#ifndef NO_EEPROM_WRITES
  EEPROMClass::write(INDEX_VERSION, version);
#else
  fakeEeprom[0] = version;
#endif

  // -1 because the last one is a placeholder
  int index;
  for (index = 0; index < amountOfIndexes; index++) {
    const int controlBytesCount = getControlBytesCount(index);
    if (version != previousVersion) {
      clearBytesToOnes(eepromConfig[index].startIndexControlBytes, controlBytesCount);
    }

    eepromConfig[index].lastIndexRead = findIndex(eepromConfig[index], controlBytesCount);
  }
  // the last one as a placeholder to calculate the length of the last real element
  eepromConfig[index].lastIndexRead = NO_DATA;
}

int EEPROM_WearLevel::getMaxDataLength(const int idx) {
  return eepromConfig[idx + 1].startIndexControlBytes
         - eepromConfig[idx].startIndexControlBytes
         - getControlBytesCount(idx);
}

unsigned int EEPROM_WearLevel::length() {
  return amountOfIndexes;
}

uint8_t EEPROM_WearLevel::read(const int idx) {
  uint8_t value = 0;
  return get(idx, value);
}

void EEPROM_WearLevel::update(const int idx, const uint8_t val) {
  put(idx, val, true);
}

void EEPROM_WearLevel::write(const int idx, const uint8_t val) {
  put(idx, val, false);
}

int EEPROM_WearLevel::getWriteStartIndex(const int idx, const int dataLength, const byte *values, const bool update, const int controlBytesCount) {
  Serial.print(F("dataLength: "));
  Serial.println(dataLength);
  if (dataLength > getMaxDataLength(idx)) {
    Serial.print(F("dataLength too long. Max: "));
    Serial.print(getMaxDataLength(idx));
    Serial.print(F(", is: "));
    Serial.println(dataLength);
    return -2;
  }

  EepromConfig &config = eepromConfig[idx];
  int previousLastIndex = config.lastIndexRead;
  if (previousLastIndex == NO_DATA) {
    // no data yet so we set the index to one before the first index
    previousLastIndex = config.startIndexControlBytes + controlBytesCount - 1;
  }
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
      return -3;
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
  return newStartIndex;
}

void EEPROM_WearLevel::updateControlBytes(int idx, int newStartIndex, int dataLength, const int controlBytesCount) {
  EepromConfig &config = eepromConfig[idx];
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
      Serial.print(F("write in loop, index: "));
      Serial.println(config.startIndexControlBytes + controlByteIndex);
      programZeroBitsToZero(config.startIndexControlBytes + controlByteIndex, writeMask, 2);
      writeMask = 0xFF;
      newBitPosInControlByte = 0;
      controlByteIndex++;
    }
  }
  if (writeMask != 0xFF) {
    Serial.print(F("after loop, index: "));
    Serial.println(config.startIndexControlBytes + controlByteIndex);
    programZeroBitsToZero(config.startIndexControlBytes + controlByteIndex, writeMask, 2);
  }
}

void EEPROM_WearLevel::printBinary(int startIndex, int endIndex) {
  for (int i = startIndex; i <= endIndex; i++) {
    const byte value = readByte(i);
    Serial.print(i);
    Serial.print(F(":"));
    printBinWithLeadingZeros(value);
    Serial.print(F("/"));
    Serial.print(value);
    Serial.print(F(" "));
    // +1 to put line break before 10, 20..
    if ((i + 1) % 10 == 0) {
      Serial.println();
    }
  }
  Serial.println();
}

void EEPROM_WearLevel::printStatus() {
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

const int EEPROM_WearLevel::getControlBytesCount(const int idx) {
  const int length = eepromConfig[idx + 1].startIndexControlBytes - eepromConfig[idx].startIndexControlBytes;
  if ((length % 9) == 0) {
    // div exact
    return length / 9;
  } else {
    // div not exact so we need to round up
    return length / 9 + 1;
  }
}

const int EEPROM_WearLevel::findIndex(const EepromConfig &config, const int controlBytesCount) {
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
const int EEPROM_WearLevel::findControlByteIndex(const int startIndex, const int length) {
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

const inline byte EEPROM_WearLevel::readByte(const int index) {
#ifndef NO_EEPROM_WRITES
  return EEPROMClass::read(index);
#else
  return fakeEeprom[index];
#endif
}


// http://www.tronix.io/data/avratmega/eeprom/
void EEPROM_WearLevel::programBitToZero(int index, byte bitIndex) {
  byte value = 0xFF;
  value &= ~(1 << (7 - bitIndex));
  Serial.print(F("writeBite: "));
  printBinWithLeadingZeros(value);
  Serial.println();
  programZeroBitsToZero(index, value);
}

void EEPROM_WearLevel::programZeroBitsToZero(int index, byte byteWithZeros, int retryCount) {
  do {
    Serial.print(F("byteWithZeros: "));
    printBinWithLeadingZeros(byteWithZeros);
    Serial.println();
    programZeroBitsToZero(index, byteWithZeros);
    retryCount--;
    Serial.print(F("eeprom is: "));
    printBinWithLeadingZeros(readByte(index));
    Serial.println();
    // byteWithZeros ^ 0xFF inverts all bits of byteWithZeros
  } while (retryCount > 0 && (readByte(index) & (byteWithZeros ^ 0xFF)) != 0);
}

#ifdef NO_EEPROM_WRITES
// emulate EEPROM behaviour to program only bits that are 0
void EEPROM_WearLevel::programZeroBitsToZero(int index, byte byteWithZeros) {
  byte mask = 1;
  int bitPosInByte = 7;
  // do while bit still in mask
  while (mask != 0) {
    if ((byteWithZeros & mask) == 0) {
      fakeEeprom[index] &= ~(1 << (7 - bitPosInByte));
      Serial.println(bitPosInByte);
    }
    mask <<= 1;
    bitPosInByte--;
  }
}
#else
void EEPROM_WearLevel::programZeroBitsToZero(int index, byte byteWithZeros) {
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

  // Wait for completion of write.
  while (EECR & (1 << EEPE));
}
#endif

void EEPROM_WearLevel::clearBytesToOnes(int fromIndex, int length) {
  for (int i = fromIndex; i < fromIndex + length; i++) {
    if (readByte(i) != 0xFF) {
#ifndef NO_EEPROM_WRITES
      clearByteToOnes(i);
#else
      fakeEeprom[i] = 0xFF;
#endif
      Serial.print(F("clear byte: "));
      Serial.println(i);
    }
  }
}

void EEPROM_WearLevel::clearByteToOnes(int index) {
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

void EEPROM_WearLevel::printBinWithLeadingZeros(byte value) {
  byte mask = 1 << 7;
  for (int bitPosInByte = 0; bitPosInByte < 8; bitPosInByte++) {
    Serial.print((value & mask) != 0 ? 1 : 0);
    mask >>= 1;
  }
}

