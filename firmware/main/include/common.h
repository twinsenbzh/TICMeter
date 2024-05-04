#ifndef COMMON_H
#define COMMON_H

#include "linky.h"
#include "config.h"

void delete_task(TaskHandle_t task);
void suspend_task(TaskHandle_t task);
void resume_task(TaskHandle_t task);
void hard_restart();
void remove_char(char *str, char c);
#endif