#include "LoRaCoAP.h"

#ifdef DUMP_COAP
char *TypeMsg[] = { "CON", "NON", "ACK", "RST" };
char *TypeCode[] = { "PING", "GET", "POST", "PUT", "DELETE" };
#endif


// to send a packet we store in an Arduino buffer before doing
// a lora.write. May be it is possible to send byte per byto to the 
// the lora shield to avoid buffering.

byte outgoingBuf[200];   // 50 is the max size, NO CHECK!!!!!
int  idxOB = 0;        

#define MAX_IB 100
byte incomingBuf[MAX_IB]; 
int  idxIB = 0;           //index to read the buffer
int  IBlength = 0;       // current size

CoAPServer::CoAPServer()
{
  resList = new CoAPResource ();

  s_mid = 1;
}

void CoAPServer::begin(LoraShield l, String n)
{
  l.init();
  Serial.println("Lorashield initailized");
  l.begin(n);
  Serial.print ("Registering "); Serial.println(n);  

  lora = l;
}

uint8_t CoAPServer::processRequest ()
{

  String URIPath = "";
  String input = ""; // data in request
  int option = 0, format = 0;
  uint16_t currentDelta;
  byte b;
  uint8_t status = 0xFF;
  uint8_t type;
  int tkl;
  uint8_t code;
  uint16_t mid;
  int cla, detail; 
  byte tok[8];
  CoAPResource *URIres;
  CoAPToken token;

  b = readByte ();  // first byte

  int version = (b & 0xC0) >> 6;

  Serial.print(">>>"); Serial.print(b, HEX); Serial.print (" "); Serial.println(version); 


  if (version != 1) { // not CoAP skip
#ifdef DUMP_COAP
    Serial.println ("not a CoAP v1 msg");
#endif 
    status = 0xFE; 
    goto not_coap_err;
   }

  type = (b & 0x30) >> 4;
  tkl = (b & 0x0F);

  b = readByte (); // second byte

  code = b;

  mid = (readByte () << 8) | readByte (); // MID


#ifdef DUMP_COAP
  Serial.print ("ver:");
  Serial.print (version);
  Serial.print (" Type = ");
  Serial.print (type);
  Serial.print (" (");
  Serial.print ((type < 4) ? TypeMsg[type] : "???");
  Serial.print (")");
  Serial.print (" Token Length = ");
  Serial.print (tkl);
  Serial.print (" code ");
  Serial.print (code);
#endif
    cla = (code & 0xE0) >> 5;
    detail = code & 0x1F;

#ifdef DUMP_COAP
  Serial.print (" (");
  if (!cla)
    {
      Serial.print ((type < 4) ? TypeCode[detail] : "???");
    }
  else
    {
      Serial.print (cla);
      Serial.print (".");
      Serial.print (detail);
    }
  Serial.print (")");
  Serial.print (" Msg id = ");
  Serial.println (mid, HEX);
#endif

 
  for (int k=0; k <tkl; k++) 
    tok[k] = readByte();
  
  token.setToken (tkl, tok);

#ifdef DUMP_COAP
  Serial.print ("Token: ");
  token.printToken ();
#endif
  // process Options

  b = 0x00;

  while ((idxIB < IBlength) && (b != 0xFF))
    {
      b = readByte ();
      if (b != 0xFF)
	{
	  uint32_t delta = (b & 0xF0) >> 4;
	  uint32_t length = (b & 0x0F);

	  if (delta == 0x0F)
	    {
	      if (length = 0x0F)
		break;
	      else
		{
		  status = ERROR_CODE (4, 02);
		  goto proc_error;
		};
	    }

	  if ((delta == 13) || (delta == 14))
	    {
	      uint16_t
		d1 = readByte () + 13;
	      if (delta == 14)
		d1 = d1 * 256 + readByte () + 256;

	      delta = d1;
	    }

	  option += delta;

	  if ((length == 13) || (length == 14))
	    {
	      uint16_t
		l1 = readByte () + 13;
	      if (delta == 14)
		l1 = l1 * 256 + readByte () + 256;

	      length = l1;
	    }
#ifdef DUMP_COAP
	  Serial.print ("option :");
	  Serial.print (option);
	  Serial.print (" length=");
	  Serial.println (length);
#endif
	  switch (option)
	    {
	    case COAP_OPTION_URI_PATH:
#ifdef DUMP_COAP
	      Serial.print ("Uri-Path ");
#endif
	      URIPath += '/';
	      for (int k = 0; k < length; k++)
		{
		  URIPath += (char) readByte ();
		  if (URIPath.length () > URI_LENGTH)
		    {
		      return ERROR_CODE (5, 00);
		    }
		}
#ifdef DUMP_COAP
	      Serial.println (URIPath);
#endif
	      break;
	    case COAP_OPTION_CONTENT:
	      format = getValue (length);
#ifdef DUMP_COAP
	      Serial.print ("Content-Format ");
	      displayMime (format);
#endif
	      break;


	    default:		// flush option
#ifdef DUMP_COAP
	      Serial.print ("Unknown Option :");
	      Serial.println (option);
#endif
	      for (int k = 0; k < length; k++)
		{
		  b = readByte ();
		}
	      break;
	     
	    }
#ifdef DUMP_COAP
	  Serial.println ();
#endif
	}			// b != 0xFF

    }				// while for options
  if (b == 0xFF) // store data (if exists) in input buffer
    {
      while (idxIB < IBlength)
	input += (char) readByte();
    }

  // filter wrong type 

  if (type == 0x03)
    {				// RST disable observe ?
      Serial.println ("RST MID =");
      Serial.println (mid);
    }


  if (type == 0x02)
    return 0xFF;		// should never receive an ACK

  if ((type != 0x00) && (type != 0x01))
    {				// not CON or NON
      status = ERROR_CODE (4, 00);
      goto proc_error;
    }

  // process method

  switch (code)
    {				// method
    case COAP_METHOD_GET:
      URIres = resList->find(URIPath, CR_READ);

      if (!URIres)
	{
	  status = ERROR_CODE (4, 04);
	  goto proc_error;
	}


        setHeader (((type == COAP_TYPE_CON) ? COAP_TYPE_ACK : COAP_TYPE_NON),
		   ERROR_CODE (2, 05), 
		   ((type == COAP_TYPE_CON) ? mid : s_mid++), 
		   &token);	//ACK same MID, NON new
	addOption (COAP_OPTION_CONTENT, format);
	addOption (COAP_END_OPTION);
	addValue (URIres, 0); // change 0 BY FORMAT
	endMessage ();	
	break; // end of METHOD_GET

    case COAP_METHOD_POST:
    case COAP_METHOD_PUT:
      URIres = resList->find (URIPath, CR_WRITE);

      if (!URIres)
	{
	  status = ERROR_CODE (4, 04);
	  goto proc_error;
	}

      status = (*URIres->function_put) (URIres, format, code,  input);
      goto proc_error;
 

    }

 not_coap_err:
  Serial.println("Skipping...");
  while (idxIB < IBlength)  {
  readByte(); 
  }
  Serial.println();

  return status;

 
 proc_error:
#ifdef DUMP_COAP
  Serial.print ("Send Status = ");
  Serial.println (status, HEX);
#endif
  if (status)
    {
      setHeader ( ((type == COAP_TYPE_CON) ? COAP_TYPE_ACK : COAP_TYPE_NON),
		 status,
		  ((type == COAP_TYPE_CON) ? mid : s_mid++),
		  &token);
      endMessage ();

    }
  return status;
}


void DumpPDU(byte*, uint8_t); 

int CoAPServer::incoming()
{
  uint8_t sizePacket;
  
  if (sizePacket = lora.dataAvailable()) {
    Serial.print(sizePacket); Serial.println(" Data received"); 
    if (IBlength) {
      Serial.println("Reseting reception buffer!");
      IBlength=0;
      idxIB=0;
    }
    

    for (int i = 0; i < sizePacket; i++) {
      digitalWrite(SS_PIN,LOW);
    //  READ BYTE CMD
      int previous_cmd_status = SPI.transfer(ARDUINO_CMD_READ);
      delayMicroseconds(WAIT_TIME_BETWEEN_BYTES_SPI);
      int shield_status = SPI.transfer(ARDUINO_CMD_AVAILABLE);
      delayMicroseconds(WAIT_TIME_BETWEEN_BYTES_SPI);
      if (IBlength < MAX_IB) 
        incomingBuf[IBlength++] = SPI.transfer(ARDUINO_CMD_AVAILABLE);
          else 
           SPI.transfer(ARDUINO_CMD_AVAILABLE);
      delayMicroseconds(WAIT_TIME_BETWEEN_BYTES_SPI);
      digitalWrite(SS_PIN,HIGH);
      delayMicroseconds(WAIT_TIME_BETWEEN_SPI_MSG);
    }
    #ifdef DUMP_COAP
    DumpPDU(incomingBuf, IBlength);
    #endif

    if ((incomingBuf[0] & 0xC0) == 0x40) processRequest();
    else {
        IBlength=0;
        idxIB=0;
    }
  }
}

uint8_t CoAPServer::getResult(CoAPToken* tk)
{
}

byte CoAPServer::readByte()
{
  byte b;
  

  if (IBlength) { // we have data 
     byte b=  incomingBuf[idxIB++];
     if (idxIB == IBlength) { // frame is fully read, clean it
       idxIB = 0;
       IBlength = 0; 
     }
     Serial.println(); Serial.print(IBlength); Serial.print("-"); Serial.print(idxIB); Serial.print("["); Serial.print(b, HEX); Serial.print("]");
     return b;
   }
 }



uint32_t CoAPServer::getValue (uint8_t len)
{
  int
    Value = 0;

  for (int k = 0; k < len; k++)
    {
      Value <<= 8;
      Value += readByte ();
    }
  return Value;
}

void
CoAPServer::setHeader (uint8_t type, uint8_t code, uint16_t mid,
		       CoAPToken * token)
{
  byte b[4];

  currentOption = 0;

  idxOB = 0; 

  outgoingBuf[idxOB] = 0x40 | ((type << 4) & 0x30);	// Version = 01
  outgoingBuf[idxOB++] |= (token->tkl & 0x0F);
  outgoingBuf[idxOB++] = code;
  outgoingBuf[idxOB++] = (mid >> 8);
  outgoingBuf[idxOB++] = (mid & 0x00FF);

  for (int k=0; k < token->tkl; k++) outgoingBuf[idxOB++] = token->token[k];
}



void
CoAPServer::addOption (uint8_t T, uint8_t L, byte * V)
{
  byte b;

  if (T == 0xFF)
    {
      b = 0xFF;

      outgoingBuf[idxOB++] = b;

      return;
    }

  if (T < currentOption)
    return;			// only increase

  uint8_t delta = T - currentOption;

  currentOption = T;

  if (delta < 0x0F)
    {
      b = (delta << 4);
    }
  else
    Serial.println ("not implemented yet");

  if (L < 0x0F)
    {
      b |= L;
    }
  else
    Serial.println ("not implemented yet");

  outgoingBuf[idxOB++] = b;
  for (int k = 0; k < L; k++) outgoingBuf[idxOB++] = V[k];
}

void
CoAPServer::addOption (uint8_t T, uint32_t V)
{
  byte b;
  uint8_t L;
  bool opt = true; 

  if (T == 0xFF)
    {
      b = 0xFF;
      outgoingBuf[idxOB++] = b;

      return;
    }

  if (T < currentOption)
    return;			// only increase

  uint8_t delta = T - currentOption;

  currentOption = T;

  byte O[4];
  uint32_t mask = 0xFF000000;
  L = 4;

  for (int k = 0; k < 4; k++)
    {
      O[k] = ((V & mask) >> (3 - k) * 8);
      mask >>= 8;
      if (!O[k] & opt) 
	L--;
      else opt = false;
    }

  if (!L)
    L = 1;			// send a least 1 byte

  if (delta < 0x0F)
    {
      b = (delta << 4);
    }
  else
    Serial.println ("not implemented yet");

  if (L < 0x0F)
    {
      b |= L;
    }
  else
    Serial.println ("not implemented yet");

  outgoingBuf[idxOB++] = b;
  for (int k = 4 - L; k < 4; k++)
    outgoingBuf[idxOB++] = O[k];
}

void
CoAPServer::addValue (CoAPResource* res, uint8_t format)
{
  String result = (*res->function_get) (res, format);

  for (int k =0; k < result.length(); k++) outgoingBuf[idxOB++] = result[k];
}

void
CoAPServer::addValue (char* txt, uint8_t format)
{

  int lgt = strlen("txt");
  for (int k =0; k < lgt; k++) outgoingBuf[idxOB++] = txt[k];
}

void
CoAPServer::addValue (String s, uint8_t format)
{

  int lgt = s.length();
  for (int k =0; k < lgt; k++) outgoingBuf[idxOB++] = s[k];
}

void
CoAPServer::endMessage ()
{
#ifdef DEEP_DEBUG
  Serial.println ("Sending...");
  for (int i=0; i < idxOB; i++) {
    Serial.print (outgoingBuf[i], HEX);
    Serial.print (" ");
  }
#endif
  Serial.println();

  lora.write (outgoingBuf, idxOB); 
}

//================

CoAPResource* 
CoAPResource::add (String newName, t_answer_get fg, t_answer_put fp, uint8_t type)
{
  CoAPResource *NE = this;


  while (NE->next != NULL)
    {
      if (NE->name == newName)
	{
	  return NE;
	}
      NE = NE->next;
    }
  NE->next = new CoAPResource (newName, (t_answer_get) fg, fp, type);
  return NE->next;
}


#ifdef DUMP_COAP
void
CoAPResource::list ()
{
  Serial.println ("list ");

  CoAPResource *NE = this;
  while (NE != NULL)
    {
      Serial.println (NE->name);
      NE = NE->next;
    }
}
#endif

CoAPResource *
CoAPResource::find (String targetName, uint8_t type)
{
  CoAPResource *current = this;

  while (current)
    {
      if ((current->name == targetName) && (current->type & type))
	return current;

      current = current->next;
    }

  return NULL;
}

#ifdef DUMP_COAP
void
CoAPServer::displayMime (int m)
{
  Serial.print ("(");
  Serial.print (m);
  Serial.print (") ");
  switch (m)
    {
    case 0:
      Serial.print ("text/plain; charset=utf-8");
      break;
    case 40:
      Serial.print ("Application/link-format");
      break;
    case 41:
      Serial.print ("Application/xml");
      break;
    case 42:
      Serial.print ("Application/octet-stream");
      break;
    case 47:
      Serial.print ("Application/exi");
      break;
    case 50:
      Serial.print ("Application/json");
      break;
    case 60:
      Serial.print ("Application/cbor");
      break;
    default:
      Serial.print ("Unknown");
    }
}
#endif
//==

void
CoAPToken::setToken (uint8_t l, byte * p)
{
  tkl = l;

  for (int k = 0; k < l; k++)
    token[k] = p[k];
}

void
CoAPToken::writeToken (byte * p)
{
  for (int k = 0; k < tkl; k++)
    p[k] = token[k];
}

bool CoAPToken::compareToken (uint8_t l, byte * p)
{
  if (tkl != l)
    return false;

  for (int k = 0; k < l; k++)
    if (token[k] != p[k])
      return false;

  return true;
}

void
CoAPToken::copy (CoAPToken * source)
{
  tkl = source->tkl;

  for (int k = 0; k < tkl; k++)
    token[k] = source->token[k];
}


#ifdef DUMP_COAP
void
CoAPToken::printToken ()
{
  for (int k = 0; k < tkl; k++)
    Serial.print (token[k], HEX);
  Serial.println ();
}

void DumpPDU (byte* p, uint8_t l) {
    int i=0; 
    int j=0;
    
    while ((i+j) < l) {
      i = 0;   
      while ((i <16) && (i+j < l)) {
        if (p[i+j]<0x10) Serial.print ("0");
        Serial.print (p[i+j], HEX); Serial.print (" ");
        if (i == 8) Serial.print (" - "); 
       
        i++; 
      }  
    
      for (int k=i; k < 16; k++) Serial.print("   ");
      if (i < 8) Serial.print("   ");
      
      i = 0;
      while ((i <16) && (i+j < l)) {
        if (p[i+j] > 32) Serial.print ((char) p[i+j]); 
                    else Serial.print (".");
        if (i == 8) Serial.print (" "); 
       
        i++; 
      }  
      Serial.println();
      j +=16; i=0;
    }  
}

#endif


//========
