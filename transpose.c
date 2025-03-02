#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>


char *read_line(void);
char *encrypt(char *plaintext, int key);
char *decrypt(char *ciphertext, int key);


int main(int argc, char *argv[]){

    printf("Plaintext: ");
    char *buffer = read_line();
    int key; 
    printf("Key length: ");
    scanf("%d", &key);
    char *cipher = encrypt(buffer, key);
    printf("Ciphertext is: %s\n", cipher);
    char *plain = decrypt(cipher, key);
    printf("Plaintext is: %s\n", plain);
    
    free(buffer);
    free(cipher);
    free(plain);
    return 0;
}



char *read_line(void){
    int length = 0; 
    int size = 10;
    char *buffer = malloc(size * sizeof(char));
    if (!buffer){
        fprintf(stderr, "Failed to allocate memory.");
        exit(1);
    }
    char c;
    while((c = getchar()) != '\n' && c != EOF){
        if (length + 1  > size){
            size *= 2;
            char *temp = realloc(buffer, size * sizeof(char));
            if (!temp){
                fprintf(stderr, "Failed to reallocate memory.");
                free(buffer);
                exit(1);
            }
            buffer = temp;

        }

        buffer[length++] = c; 


    }
    buffer[length] = 0;
    return buffer; 

}


char *encrypt(char *plaintext, int key){
    if (key <= 0 || strlen(plaintext) <=  key){
        fprintf(stderr, "Plaintext must be longer than key.");
        exit(1);
    }
    char *cipher; 
    cipher = malloc((strlen(plaintext) + 1) * sizeof(char));
    if (!cipher){
        fprintf(stderr, "Failed to allocate memory.");
        free(plaintext);
        exit(1);
    }

    int column = 0;
    int counter = 0; 
    while(column < key){
        int position = column;
        while(1){
            cipher[counter] = plaintext[position];
            counter++;
            position += key;
            if (position >= strlen(plaintext)) break;
        }
        column++;         
    }
    cipher[counter] = 0;
    return cipher;

}



char *decrypt(char *ciphertext, int key){
    if (key <= 0 || key >= strlen(ciphertext)){
        fprintf(stderr, "Ciphertext must be longer than key.");
        free(ciphertext);
        exit(1);
    }

    char *plaintext;
    plaintext = malloc((strlen(ciphertext) + 1) * sizeof(char));
    if (!plaintext){
        fprintf(stderr, "Failed to allocate memory.");
        free(ciphertext);
        exit(1);
    } 

    
    int column, row = 0; 
    int counter = 0;
    int position;
    int height = strlen(ciphertext) / key;
    int remainder = strlen(ciphertext) % key;
    if (remainder) height++;
    while(row < height){ 
        position = row;
        column = 0;
        while(column < key){
            plaintext[counter] = ciphertext[position]; 
            if (!remainder || column < remainder) position += height;
            else position += (height - 1);
            column++;
            counter++;
            if (row == (height  - 1) && remainder && column >= remainder) break;
        }
        row++;        
    }  
    

    plaintext[counter] = 0;
    return plaintext; 


}