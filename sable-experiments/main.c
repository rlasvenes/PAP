#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define SIZE 9

#define ITERATIONS 1

int table[SIZE][SIZE];
char line[100];

void afficher_table();
int traiter();

int traiter() {
    int i,j;
    int changement = 0;
    for (i = 0; i < SIZE-1; i++) {
        for (j = 0; j < SIZE-1; j++) {
            if (table[i][j] >= 4) {
                int mod4 = table[i][j] % 4;
                int div4 = table[i][j] / 4;
                
                table[i][j] = mod4;
                table[i-1][j] += div4;
                table[i+1][j] += div4;
                table[i][j-1] += div4;
                table[i][j+1] += div4;
                
                changement = 1;
            }
        }
    }
    return changement;
}

void afficher_table() {
    for (int i = 0; i < SIZE-1; i++) {
        for (int j = 0; j < SIZE-1; j++) {
            printf("%d ", table[i][j]);
        }
        printf("\n");
    }
}

void quatre_partout() {
    for (int i = 0; i < SIZE-1; i++) {
        for (int j = 0; j < SIZE-1; j++) {
            table[i][j] = 4;
        }
    }
}

int main(int argc, char const *argv[])
{
    int i = 0;
    quatre_partout();

    do {
        printf("======== itération %d ========\n", i++);
        afficher_table();
    } while (traiter());
    

    return 0;
}