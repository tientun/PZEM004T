#include "PZEM004T.h"

#define PZEM_VOLTAGE (uint8_t)0xB0
#define RESP_VOLTAGE (uint8_t)0xA0

#define PZEM_CURRENT (uint8_t)0xB1
#define RESP_CURRENT (uint8_t)0xA1

#define PZEM_POWER   (uint8_t)0xB2
#define RESP_POWER   (uint8_t)0xA2

#define PZEM_ENERGY  (uint8_t)0xB3
#define RESP_ENERGY  (uint8_t)0xA3

#define PZEM_SET_ADDRESS (uint8_t)0xB4
#define RESP_SET_ADDRESS (uint8_t)0xA4

#define PZEM_POWER_ALARM (uint8_t)0xB5
#define RESP_POWER_ALARM (uint8_t)0xA5

#define RESPONSE_SIZE sizeof(PZEMCommand)
#define RESPONSE_DATA_SIZE RESPONSE_SIZE - 2

#define READ_TIMEOUT 1000


PZEM004T::PZEM004T(uint8_t receivePin, uint8_t transmitPin)
    : serial(receivePin, transmitPin)
{
    serial.begin(9600);
}

float PZEM004T::voltage(const IPAddress &addr)
{
    send(addr, PZEM_VOLTAGE);
    uint8_t data[RESPONSE_DATA_SIZE];
    if(!recieve(data, RESP_VOLTAGE))
        return -1.0;

    return (data[0] << 8) + data[1] + (data[2] / 10.0);
}

float PZEM004T::current(const IPAddress &addr)
{
    send(addr, PZEM_CURRENT);
    uint8_t data[RESPONSE_DATA_SIZE];
    if(!recieve(data, RESP_CURRENT))
        return -1.0;

    return (data[0] << 8) + data[1] + (data[2] / 100.0);
}

float PZEM004T::power(const IPAddress &addr)
{
    send(addr, PZEM_POWER);
    uint8_t data[RESPONSE_DATA_SIZE];
    if(!recieve(data, RESP_POWER))
        return -1.0;

    return (data[0] << 8) + data[1];
}

float PZEM004T::energy(const IPAddress &addr)
{
    send(addr, PZEM_ENERGY);
    uint8_t data[RESPONSE_DATA_SIZE];
    if(!recieve(data, RESP_ENERGY))
        return -1.0;

    return (data[0] << 16) + (data[1] << 8) + data[2];
}

bool PZEM004T::setAddress(const IPAddress &newAddr)
{
    send(newAddr, PZEM_SET_ADDRESS);
    uint8_t data[RESPONSE_DATA_SIZE];
    if(!recieve(data, RESP_SET_ADDRESS))
        return false;

    return true;
}

bool PZEM004T::setPowerAlarm(const IPAddress &addr, uint8_t threshold)
{
    send(addr, PZEM_POWER_ALARM, threshold);
    uint8_t data[RESPONSE_DATA_SIZE];
    if(!recieve(data, RESP_POWER_ALARM))
        return false;

    return true;
}

void PZEM004T::send(const IPAddress &addr, uint8_t cmd, uint8_t data)
{
    PZEMCommand pzem;

    pzem.command = cmd;
    for(int i=0; i<sizeof(pzem.addr); i++)
        pzem.addr[i] = addr[i];
    pzem.data = data;

    uint8_t *bytes = (uint8_t*)&pzem;
    pzem.crc = crc(bytes, sizeof(pzem) - 1);

    serial.write(bytes, sizeof(pzem));
}

bool PZEM004T::recieve(uint8_t *data, uint8_t resp)
{
    char buffer[RESPONSE_SIZE];
    int len = 0;

    unsigned long startTime = millis();
    while((len < RESPONSE_SIZE) && (millis() - startTime < READ_TIMEOUT))
    {
        if(serial.available() > 0)
        {
            char c = serial.read();
            if(!c && !len)
                continue; // skip 00 at startup
            buffer[len++] = c;
        }
    }

    if(len != RESPONSE_SIZE)
        return false;

    if(buffer[6] != (char)crc((uint8_t*)&buffer, len - 1))
        return false;

    if(buffer[0] != (char)resp)
        return false;

    for(int i=0; i<RESPONSE_DATA_SIZE; i++)
        data[i] = (uint8_t)buffer[1 + i];

    return true;
}

uint8_t PZEM004T::crc(uint8_t *data, uint8_t sz)
{
    uint16_t crc = 0;
    for(uint8_t i=0; i<sz; i++)
        crc += *data++;
    return (uint8_t)(crc & 0xFF);
}