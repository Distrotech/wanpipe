/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>

#define MAX_INTERFACES 16

struct interface_info
{
  char name[12];

  unsigned long tx_bytes_old;
  unsigned long rx_bytes_old;
  unsigned long tx_bytes_new;
  unsigned long rx_bytes_new;
  unsigned long tx_kbytes_dif;
  unsigned long rx_kbytes_dif;

  struct timeval time_old;  
  struct timeval time_new;

  unsigned long time_diff_ms;

  unsigned long tx_rate_whole;
  unsigned long rx_rate_whole;
  unsigned long tot_rate_whole;
  unsigned tx_rate_part;
  unsigned rx_rate_part;
  unsigned tot_rate_part;
};

unsigned long bwm_calc_remainder(unsigned long num, unsigned long den);

int main(int argc, char *argv[])
{
  FILE *devfile;

  char filename[256] = "/proc/net/dev";
  char buffer[256];
  char *buffer_pointer;

  int inum;
  int field_number;
  int total_counter;
  int sleep_time = 2;
  int first_pass = 1;

  unsigned long int conv_field;

  struct interface_info interface[MAX_INTERFACES];
  struct timezone tz;

  unsigned long rx_bw_total_whole = 0;
  unsigned long tx_bw_total_whole = 0;
  unsigned long tot_bw_total_whole = 0;
  unsigned rx_bw_total_part = 0;
  unsigned tx_bw_total_part = 0;
  unsigned tot_bw_total_part = 0;

  if(argc >= 2) if((sleep_time = atoi(argv[1])) < 1) sleep_time = 1;
  if(argc >= 3) strncpy(filename, argv[2], 255);

  if(argc > 3) 
  {
    printf("\nUsage: %s <update time in seconds> <filename>\n\n", argv[0]);
    printf("Update time defaults to 2 seconds if not specified.  Minimum is 1 second.\n");
    printf("Filename defaults to /proc/net/dev if not specified.\n");
    exit(EXIT_FAILURE);
  }

  printf("%c[2J",27);

  while(1)
  {
    printf("%c[H",27);
    printf("Bandwidth Monitor 1.1.0\n\n");
    printf("       Iface        RX(KB/sec)   TX(KB/sec)   Total(KB/sec)\n\n");

    inum = -1;

    if((devfile = fopen(filename, "r")) == NULL) 
    {
      perror("fopen");
      exit(EXIT_FAILURE);
    }

    fgets(buffer, 255, devfile);

    while(fgets(buffer, 255, devfile) != NULL && inum++ < MAX_INTERFACES - 1)
    {
      interface[inum].time_old = interface[inum].time_new;
      gettimeofday(&interface[inum].time_new, &tz);

      interface[inum].time_diff_ms =
       (unsigned long)(interface[inum].time_new.tv_sec * 1000 +
        interface[inum].time_new.tv_usec / 1000) -
         (interface[inum].time_old.tv_sec * 1000 +
           interface[inum].time_old.tv_usec / 1000);

      if(inum > 0)
      {
        buffer_pointer = buffer;
        buffer_pointer = strtok(buffer_pointer, " :");
        strncpy(interface[inum].name, buffer_pointer, 11);

        field_number = 0;

        while((buffer_pointer = strtok(NULL, " :")) != NULL)
        {
          conv_field = strtoul(buffer_pointer, NULL, 10);

          field_number++;

          switch(field_number)
          {
            case 1: 
            {
              interface[inum].rx_bytes_old = interface[inum].rx_bytes_new;
              interface[inum].rx_bytes_new = conv_field;

              interface[inum].rx_kbytes_dif =
               (interface[inum].rx_bytes_new -
                interface[inum].rx_bytes_old) * 1000 / 1024;

              interface[inum].rx_rate_whole = 
               interface[inum].rx_kbytes_dif / 
                interface[inum].time_diff_ms;

              interface[inum].rx_rate_part = 
               bwm_calc_remainder(interface[inum].rx_kbytes_dif,
                interface[inum].time_diff_ms);
            }
            break;
   
            case 9:
            {
              interface[inum].tx_bytes_old = interface[inum].tx_bytes_new;
              interface[inum].tx_bytes_new = conv_field;

              interface[inum].tx_kbytes_dif =
               (interface[inum].tx_bytes_new -
                interface[inum].tx_bytes_old) * 1000 / 1024;

              interface[inum].tx_rate_whole = 
               interface[inum].tx_kbytes_dif / 
                interface[inum].time_diff_ms;

              interface[inum].tx_rate_part = 
               bwm_calc_remainder(interface[inum].tx_kbytes_dif,
                interface[inum].time_diff_ms);

              interface[inum].tot_rate_whole =
               interface[inum].rx_rate_whole +
                interface[inum].tx_rate_whole;

              interface[inum].tot_rate_part =
               interface[inum].rx_rate_part +
                interface[inum].tx_rate_part;

              if(interface[inum].tot_rate_part > 1000)
              {
                interface[inum].tot_rate_whole++;
                interface[inum].tot_rate_part -= 1000;
              }
            }
            break;
          }
        }
   
        if(!first_pass)
        {
          printf("%12s     %8lu.%03u %8lu.%03u    %8lu.%03u\n", 
            interface[inum].name,
             interface[inum].rx_rate_whole, interface[inum].rx_rate_part, 
              interface[inum].tx_rate_whole, interface[inum].tx_rate_part, 
               interface[inum].tot_rate_whole, 
                interface[inum].tot_rate_part);
        }
      }
    }

    fclose(devfile);

    rx_bw_total_whole = 0;
    tx_bw_total_whole = 0;
    rx_bw_total_part = 0;
    tx_bw_total_part = 0;

    for(total_counter = 1; total_counter <= inum; total_counter++)
    {
      rx_bw_total_whole += interface[total_counter].rx_rate_whole;
      rx_bw_total_part += interface[total_counter].rx_rate_part;

      if(rx_bw_total_part > 1000)
      {
        rx_bw_total_whole++;
        rx_bw_total_part -= 1000;
      }

      tx_bw_total_whole += interface[total_counter].tx_rate_whole;
      tx_bw_total_part += interface[total_counter].tx_rate_part;

      if(tx_bw_total_part > 1000)
      {
        tx_bw_total_whole++;
        tx_bw_total_part -= 1000;
      }

      tot_bw_total_whole = rx_bw_total_whole + tx_bw_total_whole;
      tot_bw_total_part = rx_bw_total_part + tx_bw_total_part;
    
      if(tot_bw_total_part > 1000)
      {
        tot_bw_total_whole++;
        tot_bw_total_part -= 1000;
      }
    }

    if(inum < 1) printf("No interfaces!\n\n");

    if(!first_pass)
    {
      printf("\n       Total     %8lu.%03u %8lu.%03u    %8lu.%03u\n\n",
       rx_bw_total_whole, rx_bw_total_part,
        tx_bw_total_whole, tx_bw_total_part,
         tot_bw_total_whole, tot_bw_total_part);

      printf("Hit CTRL-C to end this madness.\n");
    }

    sleep(sleep_time);
    first_pass = 0;
  }

  exit(EXIT_SUCCESS);
}

unsigned long bwm_calc_remainder(unsigned long num, unsigned long den)
{
  unsigned long long n = num;
  unsigned long long d = den;

  return (((n - (n / d) * d) * 1000) / d);
}
