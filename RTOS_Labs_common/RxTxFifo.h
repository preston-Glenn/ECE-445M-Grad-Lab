

#define RxTxFIFOSIZE   1024       // size of the FIFOs (must be power of 2)
#define RxTxFIFOSUCCESS 1         // return value on success
#define RxTxFIFOFAIL    0         // return value on failure


// RX
void RxFifo_Init(void);
int RxFifo_Put (char data);
int RxFifo_Get (char *datapt);
unsigned short RxFifo_Size (void);

// TX

void TxFifo_Init(void);
int TxFifo_Put (char data);
int TxFifo_Get (char *datapt);
unsigned short TxFifo_Size (void);
