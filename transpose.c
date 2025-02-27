#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>


char *read_line(void);
char *encrypt(char *plaintext, int key);



int main(int argc, char *argv[]){

    char *buffer = read_line();
    int key; 
    scanf("%d", &key);
    char *cipher = encrypt(buffer, key);
    printf("%s", cipher);
    free(buffer);
    free(cipher);
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
    int remainder = strlen(plaintext) % key;
    int length = strlen(plaintext) / key;
    if (!remainder) --length;
    int counter = 0; 
    
    while(column < key){
        int position = column;
        int row = 0;
        while( row <= length){
            if ( row == length && column >= remainder) break;
            cipher[counter] = plaintext[position];
            position += key;
            row++;
            counter++;
        }
        column++;         
    }
    cipher[counter] = 0;
    return cipher;

}