#ifndef PHILOSOPHER_H
#define PHILOSOPHER_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

// Número de filósofos
#define N 5

// Estados possíveis
#define PENSANDO 'P'
#define FAMINTO   'F'
#define COMENDO   'C'

// Funções
void* philosopher(void* num);

#endif
