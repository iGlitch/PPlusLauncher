#ifndef NETWORKLOADER_H
#define NETWORKLOADER_H

bool Network_Init();

void Network_Start();

void * networkThreadFunction();

void Network_Stop();

#endif