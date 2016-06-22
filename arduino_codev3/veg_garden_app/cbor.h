class cbor {
  char* value; //TODO transform into byte*
  int  length;

  int  size(byte*);
 public: 
  cbor(long v);      //type 0 and 1
  cbor(long long v); //type 0 and 1
  cbor(byte* b, int l); // type 2
  cbor(String s);       // type 3
  cbor(int size,...);  // type 4 list of cbor*
  cbor(char*, int size,  ...);    // type 5 pairs
  ~cbor ();

  void print() {this->print((byte*)this->value);};
  void print(byte*);
  void dump(byte*, int); 
  void dump() { this->dump((byte*) this->value, (int)this->length); };
};

