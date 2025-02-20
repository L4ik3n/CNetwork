#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>


char *read_line(void);
void decipher(int transp, char *ciphertext);


int main(int argc, char* argv[]){

    printf("Enter cipher text:\n");
    char *ciphertext = read_line();
    int transp = 0;
    printf("Enter number of transpositions:\n");
    scanf("%d", &transp);
    decipher(transp, ciphertext);
    printf("Plain text:\n%s\n", ciphertext);
    free(ciphertext);


    return 0;
}

char *read_line(void){
    int size = 10;
    int len = 0;
    char *buffer = malloc(sizeof(char) * size);
    if (!buffer){
        fprintf(stderr, "Memory allocation failed.");
        exit(1);        
    } 

    char c;
    while((c = getchar()) != '\n' && c != EOF){
        c = toupper(c);
        if(len + 1 > size){
            size *= 2;
            char *temp = realloc(buffer, sizeof(char) * size);
            if(!temp){
                fprintf(stderr, "Memory allocation failed.");
                free(buffer);
                exit(1);
            }
            buffer = temp;
        }
        buffer[len++] = c;

    }
    buffer[len] = '\0';
    return buffer;
}


void decipher(int transp, char *ciphertext){
    char *charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int charset_len = strlen(charset); 
    int transpose = 0; 
    if (transp > 0) transpose = transp % 26;
    int i = 0;
    while(ciphertext[i] != '\0'){
        int position = 0;
        int j = 0;
        
        while(ciphertext[i] != charset[j]) ++j;
        if(transpose + j > 25) position = (transpose + j) % 26;
        else position = (transpose + j);
        if(position == 26) position = 0;
        ciphertext[i] = charset[position];
        ++i;
    }

}