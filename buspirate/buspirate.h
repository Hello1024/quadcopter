void spi_txn(unsigned char* b, int n);
void spi_txn_noreply(unsigned char* b, int n);
void spi_wait(int n);
void spi_init();

void CE_lo();
void CE_hi();
