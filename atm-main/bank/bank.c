#include "bank.h"
#include "ports.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <regex.h>
#include <limits.h> 
#include "../util/list.c"


#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

Bank* bank_create()
{
    Bank *bank = (Bank*) malloc(sizeof(Bank));
    if(bank == NULL)
    {
        perror("Could not allocate Bank");
        exit(1);
    }

    // Set up the network state
    bank->sockfd=socket(AF_INET,SOCK_DGRAM,0);

    bzero(&bank->rtr_addr,sizeof(bank->rtr_addr));
    bank->rtr_addr.sin_family = AF_INET;
    bank->rtr_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    bank->rtr_addr.sin_port=htons(ROUTER_PORT);

    bzero(&bank->bank_addr, sizeof(bank->bank_addr));
    bank->bank_addr.sin_family = AF_INET;
    bank->bank_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    bank->bank_addr.sin_port = htons(BANK_PORT);
    bind(bank->sockfd,(struct sockaddr *)&bank->bank_addr,sizeof(bank->bank_addr));

    // Set up the protocol state
    // TODO set up more, as needed
    bank->pins = list_create();
    bank->balances = list_create();
    // bank->ciplen = NULL;

    return bank;
}

void bank_free(Bank *bank)
{
    if(bank != NULL)
    {
        close(bank->sockfd);
        free(bank);
    }
}

ssize_t bank_send(Bank *bank, char *data, size_t data_len)
{
    // Returns the number of bytes sent; negative on error
    return sendto(bank->sockfd, data, data_len, 0,
                  (struct sockaddr*) &bank->rtr_addr, sizeof(bank->rtr_addr));
}

ssize_t bank_recv(Bank *bank, char *data, size_t max_data_len)
{
    // Returns the number of bytes received; negative on error
    return recvfrom(bank->sockfd, data, max_data_len, 0, NULL, NULL);
}

int file_exists(char *filename) {
    if (!fopen(filename, "r")) {
        return 0;
    } else {
        printf("Error opening BANK initialization file\n");
        return 64;
    }
}

void bank_process_local_command(Bank *bank, char *command, size_t len)
{
    // if(strcmp(command,"\n") == 0) {
    //     printf("Invalid command\n");
    //     return;
    // }

    // TODO: Implement the bank's local commands
        
    /* 
     * bank commands: 
     * create-user(0) <username>(1) <pin>(2) <balance>(3)
     * deposit(0) <username>(1) <amount>(2)
     * balance(0) <username>(1)
     */

    char *tok1, *tok2, *tok3, *tok4, *tok5;

    command[strlen(command) - 1] = '\0';

    tok1 = strtok(command, " ");

    // continue if only "Enter" is pressed 
    if (tok1 == NULL) {
        return;
    }

    // remove any newlines from user pressing "Enter"
    if (len == 0) {
        printf("Invalid command\n");
    }
    
    if (strcmp(tok1, "create-user") == 0) {
        tok2 = strtok(NULL, " ");
        tok3 = strtok(NULL, " ");
        tok4 = strtok(NULL, " ");
        tok5 = strtok(NULL, " ");

        if (tok2 != NULL && tok3 != NULL && tok4 != NULL && tok5 == NULL) {
            if (username_check(tok2) == 1 && pin_check(tok3) == 1 && (balance_check(tok4) == 1 || strcmp(tok4, "0") == 0)) {
                if(list_find(bank->balances, tok2) == -1) {
                create_user(bank, tok2, tok3, tok4);
                } else {
                    printf("Error:  user %s already exists\n", tok2);
                    return;
                }
            }
            else {
                printf("Usage:  create-user <user-name> <pin> <balance>\n");
                return;
            }
        }
        else {
            printf("Usage:  create-user <user-name> <pin> <balance>\n");
            return;
        }
    } 
    else if (strcmp(tok1, "deposit") == 0) {
        tok2 = strtok(NULL, " ");
        tok3 = strtok(NULL, " ");
        tok4 = strtok(NULL, " ");

        if (tok2 != NULL && tok3 != NULL && tok4 == NULL) {
            if (username_check(tok2) == 1 && deposit_check(tok3) == 1) {
                deposit(bank, tok2, tok3);
            }
            else {
                printf("Usage:  deposit <user-name> <amt>\n");
                return;
            }
        }        
        else {
            printf("Usage:  deposit <user-name> <amt>\n");
            return;
        }
    }
    else if (strcmp(tok1, "balance") == 0) {
        tok2 = strtok(NULL, " ");
        tok3 = strtok(NULL, " ");

        if (tok2 != NULL && tok3 == NULL) {
            if (username_check(tok2) == 1) {
                getBalance(bank, tok2);
            } 
            else {
                printf("Usage:  balance <user-name>\n");
                return;
            }
        }
        else {
                printf("Usage:  balance <user-name>\n");
                return;
        }
    } else {
        printf("Invalid command\n");
    }
    return;
}

void bank_process_remote_command(Bank *bank, char *command, size_t len)
{

    unsigned char key[32] = { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
                              0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
                              0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
                              0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31
                            };
    /* A 128 bit IV */
    unsigned char iv[16] = {  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
                              0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35
                           };

    char *cmd = strtok(command, " ");

    if(strcmp(cmd, "search") == 0) {
        char *username = strtok(NULL, " ");
        //printf("username in bank.c: %s\n", username);
        //printf("length of username in bank.c: %ld\n", strlen(username));

        int pin = list_find(bank->pins, username);
        if(pin == -1) {
            bank_send(bank, "DNE", strlen("DNE"));
        } else {
            char pin_str[5];
            snprintf(pin_str, 5, "%d", pin);
            unsigned char cipherpin[128];
            unsigned char *unsignedPin = (unsigned char *)pin_str;

            int cipherpin_len;
            cipherpin_len = encrypt(unsignedPin, strlen ((char *)unsignedPin), key, iv, cipherpin);
            
            bank_send(bank, cipherpin, cipherpin_len);
        }
    } else if(strcmp(cmd, "withdraw") == 0) {
        char *username = strtok(NULL, " ");
        if(list_find(bank->balances, username) != -1) {
            char *amt = strtok(NULL, " ");
            
            if(list_find(bank->balances, username) - atoi(amt) < 0) {
                bank_send(bank, "brokeboy", strlen("brokeboy"));
            } else {
                list_change(bank->balances, username, (list_find(bank->balances, username) - atoi(amt)));
                bank_send(bank, "richboy", strlen("richboy"));
            }
        }
    } else if(strcmp(cmd, "balance") == 0) {
        char *username = strtok(NULL, " ");
        //printf("username %s\n", username);
        if(list_find(bank->balances, username) != -1) {
            int balance_num = list_find(bank->balances, username);
            //printf("balance_num: %d\n", balance_num);
            char balance_str[2000];
            sprintf(balance_str,"%d", balance_num);
            //printf("balance_str: %s\n", balance_str);
            bank_send(bank, balance_str, strlen(balance_str));
        }
    }
    return;


    
    
    // TODO: Implement the bank side of the ATM-bank protocol

	/*
	 * The following is a toy example that simply receives a
	 * string from the ATM, prepends "Bank got: " and echoes 
	 * it back to the ATM before printing it to stdout.
	 */

	/*
    char sendline[1000];
    command[len]=0;
    sprintf(sendline, "Bank got: %s", command);
    bank_send(bank, sendline, strlen(sendline));
    printf("Received the following:\n");
    fputs(command, stdout);
	*/
}

int username_check(char *username) {
    int reg, valid;
    regex_t regex;

    if (username == NULL) {
        return 0;
    } 
    int len = strlen(username);
    if (len > 250) {
        return 0;
    }
    
    // checks if chars are valid
    reg = regcomp(&regex, "^[a-zA-Z]+$", REG_EXTENDED);
    if (reg == 0) {
        valid = regexec(&regex, username, 0, NULL, 0);
        regfree(&regex);
        if (valid == 0) {
            // valid username
            return 1;
        }
    }
    return 0;
}

int pin_check(char *pin) {
    int reg, valid;
    regex_t regex;
    if (pin == NULL) {
        return 0;
    }

    reg = regcomp(&regex, "^[0-9]{4}$", REG_EXTENDED);
    if (reg == 0) {
        // regex compiles success
        valid = regexec(&regex, pin, 0, NULL, 0);
        regfree(&regex);
        if (valid == 0) {
            return 1;
        }
    }
    return 0;
}

int balance_check(char *amt) {
    int reg, valid;
    regex_t regex;
    if (amt == NULL) {
        return 0;
    }

    reg = regcomp(&regex, "^[0-9]+$", REG_EXTENDED);
    if (reg == 0) {
        // regex compiles success
        valid = regexec(&regex, amt, 0, NULL, 0);
        regfree(&regex);
        if (valid == 0) {
            // pattern matched regex success
            long iamt = strtol(amt, NULL, 10);
            // check if the number is <= the max num that can be represented in int
            if (iamt > 0 && iamt <= INT_MAX) {
                return 1;
            }
        }
    }
    return 0;
}

int deposit_check(char *amt) {
    int reg, valid;
    regex_t regex;
    if (amt == NULL) {
        return 0;
    }

    reg = regcomp(&regex, "^[0-9]+$", REG_EXTENDED);
    if (reg == 0) {
        // regex compiles success
        valid = regexec(&regex, amt, 0, NULL, 0);
        regfree(&regex);
        if (valid == 0) {
            // pattern matched regex success
            // check if the number is <= the max num that can be represented in int
                return 1;
        }
    }
    return 0;
}

void deposit(Bank *bank, char* username, char* deposit_amt) {
    if(strlen(deposit_amt) > 10) {
        printf("Too rich for this program\n");
        return;
    }
    long balance = list_find(bank->balances, username);
    if(balance != -1) {
        balance += strtol(deposit_amt, NULL, 10);
        if(balance > (long)INT_MAX) {
            printf("Too rich for this program\n");
            return;
        }
        printf("$%d added to %s's account\n", atoi(deposit_amt), username);
        list_change(bank->balances, username, (int)balance);
    } else {
        printf("No such user\n");
    }
}

void create_user(Bank *bank, char* username, char* pin, char* balance) {

    list_add(bank->balances, username, atoi(balance));
    list_add(bank->pins, username, atoi(pin));

    char filename[strlen(username) + strlen(".card") + 1];
    strncpy(filename, username, sizeof(username));
    strncat(filename, ".card", sizeof(".card"));
    FILE *card = fopen(filename, "w");


    if (card == NULL) {
        printf("Error creating card file for user %s\n", username);
        return;
    }

    fwrite(pin , 1 , sizeof(pin) , card);
    fclose(card);

        // test commented
    // char testpin[128];
    // fgets(testpin, sizeof(testpin), card);
    // fclose(card);
    // printf("cipher pin: %s\n", cipherpin);
    // printf("testpin: %s\n", testpin);
    // unsigned char dtext[128];
    // int dlen;
    // dlen = decrypt(testpin, cipherpin_len, key, iv, dtext);
    // dtext[dlen] = '\0';
    // printf("decry pin: %s\n", dtext);

    printf("Created user %s\n", username);
}

void getBalance(Bank *bank, char *username) {
    int balance = list_find(bank->balances, username);
    if (balance != -1) {
        printf("$%d\n", (int)balance);
    } else {
        printf("No such user\n");
    }
}

// CODE FROM OPENSSL
void handleErrors(void)
{
    ERR_print_errors_fp(stderr);
    printf("hlllo\n");
    abort();
}

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
            unsigned char *iv, unsigned char *ciphertext)
{
    EVP_CIPHER_CTX *ctx;

    int len;

    int ciphertext_len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    /*
     * Initialise the encryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits
     */
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        handleErrors();

    /*
     * Provide the message to be encrypted, and obtain the encrypted output.
     * EVP_EncryptUpdate can be called multiple times if necessary
     */
    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        handleErrors();
    ciphertext_len = len;


    /*
     * Finalise the encryption. Further ciphertext bytes may be written at
     * this stage.
     */
    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        handleErrors();
    ciphertext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
            unsigned char *iv, unsigned char *plaintext)
{
    EVP_CIPHER_CTX *ctx;

    int len;

    int plaintext_len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    /*
     * Initialise the decryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits
     */
    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        handleErrors();

    /*
     * Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary.
     */
    if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        handleErrors();
    plaintext_len = len;

    /*
     * Finalise the decryption. Further plaintext bytes may be written at
     * this stage.
     */
    if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
        handleErrors();
    plaintext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return plaintext_len;
}