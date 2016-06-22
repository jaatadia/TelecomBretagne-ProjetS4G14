#include "LoRaCoAP.h"

#define SERIAL(txt, var) Serial.print (txt); Serial.println(var, HEX); 

void printByte(byte b) 
{
  if ((uint8_t) b < 16) Serial.print("0");
  Serial.print ((byte) b, HEX);
  Serial.print (" "); 
}

cbor::cbor (long long v)
{
  int neg = false;
  int pos=7;
  byte* p = (byte*) &v; 

  if (v <0) {
    neg = true;
    v=-v;
    v++;
  }

  value = new char[9];
  length = 9; 

  value[0] = (neg) ? 0x20 : 0x00;

  value[0] |= 27; //32 bits

  for (int i=0; i <8; i++) value[1+i] = p[7-i];  
 }

cbor::cbor(long v) // 64 are processed independanly
{
  int neg = false;
  int pos=3;
  int valueLength; 

  if (v <0) {
    neg = true;
    v=-v;
    v++;
  }

  uint32_t  msk = 0x000000FF;
  while ((pos) && (! (v & (msk << (pos << 3))))) {  // pos * 8
      pos--;
  }

  if (pos == 2) pos = 3; // 3 bytes align on int32 
  valueLength = (v < 24) ? 1 : (pos + 2);
  
  value = new char[valueLength];
   
  value[0] = (neg) ? 0x20 : 0x00;

  if (v < 24) {
    value[0] |= v;
    length = 1;
  }
  else {
    if (pos == 0) value[0] |= 24; //8 bits
    if (pos == 1) value[0] |= 25; //16 bits
    if (pos == 3) value[0] |= 26; //32 bits
    length = valueLength;

    int idx = 1;
    while (! (pos <0)) {
      value[idx] = (v & (msk << (pos << 3))) >> (pos << 3);
      idx++; pos--;
    }
  }
}

// limitation to a length <= 255

cbor::cbor(byte* b, int l)
{

  if (l < 24) length = l+1; else length = l+2; 
  value = new char[length];

  int idx = 0;

  if (l < 24) 
    value[idx++] = 0x40 | l;
  else {
    value[idx++] = 0x40 | 24;
    value[idx++] = l;
  }

  for (int i = 0; i < l; i++) value[idx++] = b[i];
}

cbor::cbor(String s)
{
  int l = s.length(); 
  if (l < 24) length = l+1; else length = l+2; 
  value = new char[length];

  int idx = 0;

  if (l < 24) 
    value[idx++] = 0x60 | l;
  else {
    value[idx++] = 0x60 | 24;
    value[idx++] = l;
  }

  for (int i = 0; i < l; i++) value[idx++] = s[i];
}

cbor::cbor(int size, ...)
{
  va_list arguments;
  cbor*   cb; 
  uint32_t elmSize = 0; 
  int idx;

  // compute size of the new structure
  va_start (arguments, size);
  for (int i=0; i < size; i++) {
    cb = va_arg (arguments, cbor*);
    elmSize += cb->length;
  }

  if (size < 24) {
    length = elmSize+1;
  } else if (size < 255) {
    length = elmSize+2;
  }  // Dont' process the case length is higher

  value = new char[length];
  idx = 0; 

  if (size < 24) {
    value[idx++] = 0x80 | size; 
  } else if (elmSize < 255) {
    value[idx++] = 0x80 | 24; 
    value[idx++] = size;
  }

  va_start (arguments, size);
  for (int i=0; i < size; i++) {
    cb = va_arg (arguments, cbor*);
    int len = cb->length;

    for (int i=0; i < len; i++) {
      value[idx++] = (char) cb->value[i];
    }
  }
  
}

cbor::cbor(char* s,  int size, ...)
{
  va_list arguments;
  cbor*   cb; 
  uint32_t elmSize = 0; 
  int idx;
  int nbElm = 0; 

  va_start (arguments, size);

  while (cb = va_arg (arguments, cbor*)){
    elmSize += cb->length;
    nbElm++;
  }

  if (nbElm%2) {
    Serial.println ("Error odd number");
    return;
  }
  if (nbElm/2 != size) {
    Serial.println ("be coherent in numbers you give");
    return;
  }

   if (size < 24) {
    length = elmSize+1;
  } else if (size < 255) {
    length = elmSize+2;
  }  // Dont' process the case length is higher

  value = new char[length];
  idx = 0; 

  if (size < 24) {
    value[idx++] = 0xA0 | size; 
  } else if (elmSize < 255) {
    value[idx++] = 0xA0 | 24; 
    value[idx++] = size;
  }

  va_start (arguments, size);
  for (int i=0; i < 2*size; i++) {
    cb = va_arg (arguments, cbor*);
    int len = cb->length;

    for (int i=0; i < len; i++) {
      value[idx++] = (char) cb->value[i];
    }
  }

}

cbor::~cbor()
{
  Serial.println ("destructor");
  if (value) delete value;
}

//-------------

int cbor::size(byte* ptr)
{
  int l; 
  int idx = 0; 
  int offset = 0; 
  int totalSize = 0;

    switch (ptr[idx] & 0xE0) {
    case 0x00: 
    case 0x20: 
      if ((ptr[idx]&0x1F) < 24) return 1;
      else {
	if ((ptr[idx]&0x1F) == 24) return 2;
	if ((ptr[idx]&0x1F) == 25) return 3;
	if ((ptr[idx]&0x1F) == 26) return 5;
	if ((ptr[idx]&0x1F) == 27) return 9;
	Serial.println("Error");
      }
	return -1;

    case 0x40: 
    case 0x60: 
      l = ptr[idx] & 0x1F;
      if (l < 24) return l+1;
      if (l == 24) return 1 + ptr[++idx]; 
      else if (l > 24) {
	Serial.println ("Not implemented");
	return -1;
      }
    case 0x80: 
      totalSize = 0;
      l = ptr[idx] & 0x1F; //number of elements in the array;
      if (l == 0) return 1;
      totalSize++;
      if (l == 24) {
	l = ptr[++idx];
	totalSize++;
      }
      idx++;

      while (l--) {
	offset = size(&ptr[idx]);
	totalSize += offset;
	idx+=offset;
      }
      return totalSize;
    case 0xA0: 
      totalSize = 0;
      l = ptr[idx] & 0x1F; //number of elements in the array;
      if (l == 0) return 1;
      totalSize++;
      if (l == 24) {
	l = ptr[++idx];
	totalSize++;
      }
      idx++;

      l *= 2; // pairs 2 elm per entry 
      while (l--) {
	offset = size(&ptr[idx]);
	totalSize += offset;
	idx+=offset;
      }
      return totalSize;
    case 0xC0: 
      Serial.println ("MAP of pairs");
      return -2; 
    case 0xE0: 
      return -2;
    }
}


//-------------

int shifting = 0; 

void align(int c) {
  for (int i=0; i <shifting*3; i++) Serial.print (" "); 
}

void cbor::print(byte* ptr)
{
  int idx = 0, idx2;
  long long result = 0;
  bool neg; 
  int valueLength;
  cbor* cb;
  int offset;

    switch (ptr[idx] & 0xE0) {
    case 0x00: 
    case 0x20: 
      neg =  (ptr[idx] & 0xE0); 

      if ((ptr[idx]&0x1F) < 24) {
	align(shifting); printByte (ptr[idx]);
	result = ptr[idx]&0x1F;
      }
      else {
	if ((ptr[idx]&0x1F) == 24) valueLength = 1;
	if ((ptr[idx]&0x1F) == 25) valueLength = 2;
	if ((ptr[idx]&0x1F) == 26) valueLength = 4;
	if ((ptr[idx]&0x1F) == 27) valueLength = 8;

	if (valueLength > 27) {
	  Serial.println ("error : int length not good");
	  return;
	}

	align(shifting); printByte(ptr[idx]); Serial.print ("-- "); 
	idx++;

	for (int i=0; i < valueLength; i++) {
	  result*= 256; 
	  result += ptr[idx];
	  printByte(ptr[idx]); 
	  
	  idx++;
	}

      }
      if (neg) { result--; result = -result;}

      if (valueLength < 8) { // cannot print long long
	Serial.print(" INT("); Serial.print((long) result); Serial.print(")");
      }
      
      Serial.println();
      return;

    case 0x40: 
      valueLength = ptr[idx] & 0x1F;
      align(shifting); printByte(ptr[idx]);

      if (valueLength == 24) {
	idx++;
	printByte (ptr[idx]); 
	valueLength = ptr[idx];
      }
      else if (valueLength > 24) {
	Serial.println ("Not implemented");
	return;
      }
      idx++;
      Serial.print ("-- "); 
      for (int i= 0; i<valueLength; i++) printByte (ptr [idx++]);
      Serial.print("Bit string ("); Serial.print(valueLength); Serial.println (" Byte(s))");
      return;
 
    case 0x60: 
      align(shifting); printByte(ptr[idx]); 
      valueLength = ptr[idx] & 0x1F;
      if (valueLength == 24) {
	idx++;
	printByte (ptr[idx]); 
	valueLength = ptr[idx];
      }
      else if (valueLength > 24) {
	Serial.println ("Not implemented");
	return;
      }
      idx++;
      idx2 = idx; 
      
      Serial.print ("-- "); 
      for (int i= 0; i<valueLength; i++) printByte (ptr[idx++]);
      Serial.print (" \""); 
      for (int i= 0; i<valueLength; i++) Serial.print((char) ptr[idx2++]);
      Serial.println("\"");
      return; 

    case 0x80: 
      align(shifting); printByte(ptr[idx]); 
      valueLength = ptr[idx++] & 0x1F; // not a length but number of elements
      if (valueLength == 24) {
	printByte(ptr[idx]); 
	valueLength = ptr[idx++];
      }

      Serial.print (" Array of "); Serial.print (valueLength); Serial.println (" element(s)"); 
      while (valueLength) {
	cb = (cbor*) &ptr[idx];
        offset = size(&ptr[idx]);
	
	shifting++;

	cb->print(&ptr[idx]);
	shifting--;

	valueLength--;
	idx += offset;
      }
      
      break; 
    case 0xA0: 
      align(shifting); printByte(ptr[idx]); 
      valueLength = ptr[idx++] & 0x1F; // not a length but number of elements
      if (valueLength == 24) {
	printByte(ptr[idx]); 
	valueLength = ptr[idx++];
      }

      Serial.print (" Map of "); Serial.print (valueLength); Serial.println (" pair(s)");

      while (valueLength) {
	cb = (cbor*) &ptr[idx];
        offset = size(&ptr[idx]);
	
	shifting++;

	cb->print(&ptr[idx]);
	idx += offset;
	Serial.print (" -> ");
	shifting +=2; 
	cb = (cbor*) &ptr[idx];
        offset = size(&ptr[idx]);
	cb->print(&ptr[idx]);
	shifting -=2; 

	idx += offset;

	shifting--;
	valueLength--; 
      }

      break; 
    case 0xC0: 
      Serial.println ("MAP of pairs");
      break; 
    case 0xE0: 
      Serial.println ("Positive int");
      break; 
    }
}

void cbor::dump(byte* ptr, int length)
{
  Serial.print ("Dumping "); Serial.print (length); Serial.println(" Bytes ...");
  for (int i=0; i <length; i++) {
    printByte(value [i]); 

    if (!((i+1) %8)) Serial.println();
  }
  Serial.println (); 
}
