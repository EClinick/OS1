# Makefile
CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra

all: enc_server enc_client dec_server dec_client keygen

enc_server: enc_server.c
	$(CC) $(CFLAGS) -o enc_server enc_server.c

enc_client: enc_client.c
	$(CC) $(CFLAGS) -o enc_client enc_client.c

dec_server: dec_server.c
	$(CC) $(CFLAGS) -o dec_server dec_server.c

dec_client: dec_client.c
	$(CC) $(CFLAGS) -o dec_client dec_client.c

keygen: keygen.c
	$(CC) $(CFLAGS) -o keygen keygen.c

clean:
	rm -f enc_server enc_client dec_server dec_client keygen ciphertext1 plaintext1_a ciphertext2 plaintext2_a ciphertext3 plaintext3_a ciphertext4 plaintext4_a ciphertext5 plaintext5_a key20 key70000 mytestresults mykey myshortkey
	
