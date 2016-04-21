/*
 *      dht22.c:
 *	Simple test program to test the wiringPi functions
 *	Based on the existing dht11.c
 *	Amended by technion@lolware.net
 *
 *	Additional modifications by manuel@muehlig.eu
 */

#include <wiringPi.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

#include "locking.h"

static const int MAXTIMINGS = 85;
static const int DEFAULT_TRIES = 100;
static const int DEFAULT_PIN = 7;

uint8_t sizecvt(int read)
{
  /* digitalRead() and friends from wiringpi are defined as returning a value
  < 256. However, they are returned as int() types. This is a safety function */

  if (read > 255 || read < 0)
  {
    fprintf(stderr, "Invalid data from wiringPi library\n");
    exit(EXIT_FAILURE);
  }
  return (uint8_t)read;
}

int read_dht22_dat(int pin)
{
  uint8_t laststate = HIGH;
  uint8_t j = 0, i;
  int dht22_dat[5] = {0,0,0,0,0};

  // pull pin down for >18 milliseconds (20ms)
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  delay(20);
  
  // prepare to read the pin
  pinMode(pin, INPUT);


  // detect change and read data
  for (i = 0; i < MAXTIMINGS; i++)
  {
    uint8_t counter = 0;
    while (sizecvt(digitalRead(pin)) == laststate)
    {
      counter++;
      delayMicroseconds(1);
      if (counter == 255)
      {
        break;
      }
    }
    laststate = sizecvt(digitalRead(pin));

    if (counter == 255)
    {
      break;
    }

    // ignore first 3 transitions
    if ((i >= 4) && (i%2 == 0))
    {
      // shove each bit into the storage bytes
      dht22_dat[j/8] <<= 1;
      if (counter > 16)
      {
        dht22_dat[j/8] |= 1;
      }

      j++;
    }
  }

  // check we read 40 bits (8bit x 5 ) + verify checksum in the last byte
  // print it out if data is good
  if ((j >= 40) && 
      (dht22_dat[4] == ((dht22_dat[0] + dht22_dat[1] + dht22_dat[2] + dht22_dat[3]) & 0xFF)) )
  {
    float t, h;
    h = (float)dht22_dat[0] * 256 + (float)dht22_dat[1];
    h /= 10;
    t = (float)(dht22_dat[2] & 0x7F)* 256 + (float)dht22_dat[3];
    t /= 10.0;
    if ((dht22_dat[2] & 0x80) != 0)
    {
      t *= -1;
    }

    printf("Humidity = %.2f %%\nTemperature = %.2f °C \n", h, t);
    return 1;
  }
  else
  {
    fprintf(stderr, "Data not good, skip\n");
    return 0;
  }
}

int main (int argc, char *argv[])
{
  int tries = DEFAULT_TRIES;
  int pin = DEFAULT_PIN;

  if (argc < 2)
  {
    printf ("usage: %s <pin> (<tries>)\ndescription: pin is the wiringPi pin number\nusing 7 (GPIO 4)\nOptional: tries is the number of times to try to obtain a read (default 100)",argv[0]);
  }
  else
  {
    pin = atoi(argv[1]);
  }
   

  if (argc == 3)
  {
    tries = atoi(argv[2]);
  }

  if (tries < 1)
  {
    printf("Invalid tries supplied\n");
    exit(EXIT_FAILURE);
  }

  int lockfd = open_lockfile(LOCKFILE);

  if (wiringPiSetup () == -1)
  {
    exit(EXIT_FAILURE) ;
  }
	
  if (setuid(getuid()) < 0)
  {
    perror("Dropping privileges failed\n");
    exit(EXIT_FAILURE);
  }

  while (read_dht22_dat(pin) == 0 && tries--) 
  {
    delay(3000); // wait 3sec to refresh
  }

  delay(1500);
  close_lockfile(lockfd);

  return EXIT_SUCCESS;
}
