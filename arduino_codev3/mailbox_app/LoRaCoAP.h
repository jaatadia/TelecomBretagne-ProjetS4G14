#ifndef CoAPServer_h
#define CoAPServer_h
#include "LoraShield.h"
#include <SPI.h>
#include "CoAP.h"
#include "cbor.h"

//DEEP_DEBUG Shows characters sent and received
//DUMP_COAP  Shows received COAP messages
//#define DEEP_DEBUG 1
//#define DUMP_COAP 1
#define ERROR_CODE(a, b) ((a) <<5 | (b))

#define URI_LENGTH 20

class CoAPResource;

typedef String  (*t_answer_get) (CoAPResource*, uint8_t);
typedef uint8_t (*t_answer_put) (CoAPResource*, uint8_t, uint8_t, String);
#define CR_READ 0x01
#define CR_WRITE 0x02
#define CR_DELETE 0x04

class CoAPResource {  
  friend class CoAPServer;

  CoAPResource* next;
  String        name;
  uint8_t       type;
  t_answer_get  function_get;
  t_answer_put  function_put;
  uint32_t*     intPtr;
  String        des; 
  
 public:
  CoAPResource() { name = "";  next = NULL; };
  CoAPResource(String n, t_answer_get fg, t_answer_put fp, uint8_t t) 
    {name = n; function_get = fg;  function_put = fp; type =t; next = NULL; };

  CoAPResource*  add(String, t_answer_get, t_answer_put, uint8_t);
  CoAPResource* find(String, uint8_t); 
#ifdef DUMP_COAP
  void list ();
#endif
};

class CoAPToken{
  friend class CoAPServer;

  int tkl;
  byte token[8];
 public:
  CoAPToken () { tkl = 0; };
  inline CoAPToken(uint8_t l, byte* p) {setToken (l, p);};
  inline CoAPToken (CoAPToken* t) {setToken (t->tkl, t->token); }

  void setToken (uint8_t, byte *);
  void writeToken (byte*);
  inline uint8_t getTokenLength() { return tkl; };
  inline bool compareToken (CoAPToken* t) { return compareToken(t->tkl, t->token); };
  bool compareToken (uint8_t, byte*);
  bool operator== (CoAPToken t) { 
    Serial.println("operator==");
#ifdef DUMP_COAP    
    this->printToken();
    t.printToken(); 
#endif
    compareToken (t.tkl, t.token);
  }

  void copy(CoAPToken*); 
#ifdef DUMP_COAP
  void printToken();
#endif
};

class CoAPServer {
  LoraShield lora;
  CoAPResource* resList;
  uint16_t      s_mid;
  uint16_t      currentOption;


public:
  CoAPServer ();
  void begin(LoraShield, String); 
  int  incoming();  //process data in server mode
  
  uint8_t  getResult(CoAPToken*); // wait for an answer matching the token
  
  byte readByte();
  uint8_t  processRequest();
  uint32_t getValue (uint8_t);

  CoAPResource*  addRes(String s, t_answer_get fg) 
  { return resList->add (s, fg, NULL, CR_READ); } 

  CoAPResource*  addRes(String s, t_answer_put fp)
            { return resList->add (s, NULL, fp, CR_WRITE); } 
  CoAPResource*  addRes(String s, t_answer_get fg, t_answer_put fp) 
            { return resList->add (s, fg, fp, CR_READ|CR_WRITE); } 
            
  int      post(CoAPResource*, char*);
 
  void     setHeader (uint8_t, uint8_t, uint16_t, CoAPToken*);
  void     addOption (uint8_t, uint8_t, byte*);
  void     addOption (uint8_t T, char* S) { addOption(T, strlen(S), (byte*) S);}; 
  void     addOption (uint8_t t) { addOption(t, 0, (byte*) 0); }
  void     addOption (uint8_t T, uint32_t V); // Optimize option Length
  void     addValue (CoAPResource*, uint8_t);
  void     addValue (char*, uint8_t); 
  void     addValue (String, uint8_t); 
  void     endMessage();

#ifdef DUMP_COAP
  void displayMime (int);
  void listRes() {resList->list(); }
#endif

};

#endif // CoAPServer.h
