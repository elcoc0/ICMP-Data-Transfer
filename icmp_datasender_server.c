
/*******************************************************************************
 *
 * File Name         : icmp_server.c
 * Created By        : Nodulaire
 * Creation Date     : September 9th, 2015
 * Last Change       : September 16th, 2015 at 08:33:16 AM
 * Last Changed By   : Nodulaire
 * Purpose           : ICMP Payload reception server 
 * Compile           : gcc -o icmp_server -Wall -lcrypto icmp_server.c
 * 
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <netinet/in.h> 
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <netinet/ether.h>


/* args miss usage function */
void usage(){
   fprintf(stderr, "Usage: sudo ./icmp_server > <filename>]\n");
   exit(EXIT_SUCCESS);
}

int main (int argc, char **argv)
{
    int i, ret, one, sock_eth, sock_icmp, fd, ip_len, icmp_len, icmp_data_in_len, opt;
    i = 0;
    ret = 0;
    one = 0;
    opt = 0;

    char *filename = NULL;
    unsigned char buf_incoming[5000];
    unsigned char buf_outgoing[5000];
    char payload[500];


    // OSI Model encapsulation
    struct sockaddr_in dst;
    struct ether_header *eth_hdr;
    struct ip *ip_hdr_in, *ip_hdr_out;
    struct icmp *icmp_hdr_in, *icmp_hdr_out;

    // A while/case is maybe overkill but let fyther amelioration 
    while ((opt = getopt(argc, argv, "h:")) != -1) {
            case 'h':
                 default:
                 usage();
            }
   }

    /* Gestion des erreurs */
   if ((sock_eth = socket (AF_INET, SOCK_PACKET, htons (ETH_P_ALL))) < 0)
   {
      perror ("Socket error (ethernet level)");
      exit (1);
    }
   if ((sock_icmp = socket (AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
    {
      perror ("Socket error (proto icmp level)"); 
      exit (1);
    }
   if ((ret = setsockopt (sock_icmp, IPPROTO_IP, IP_HDRINCL, (char *) &one,  sizeof (one))) < 0)
    {
      perror ("Setsockopt error");
      exit (1);
    }

   if (filename){ 
      if ((fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC|O_SYNC, S_IRUSR)) == -1){
         perror("Open/permissions error on the input file ");
         exit(1);
      }
   }
   else{
      fd = STDOUT_FILENO;
   }

   eth_hdr = (struct ether_header *) buf_incoming;

   ip_hdr_in = (struct ip *) (buf_incoming + sizeof (struct ether_header));
   icmp_hdr_in = (struct icmp *) ((unsigned char *) ip_hdr_in + sizeof (struct ip));

   ip_hdr_out = (struct ip *) buf_outgoing;
   icmp_hdr_out = (struct icmp *) (buf_outgoing + sizeof (struct ip));

   while ((ret = recv (sock_eth, buf_incoming, sizeof (buf_incoming), 0)) > 0){
      if (ip_hdr_in->ip_p == IPPROTO_ICMP){
         if (icmp_hdr_in->icmp_type == ICMP_ECHO){
            ip_hdr_out->ip_v = ip_hdr_in->ip_v;
            ip_hdr_out->ip_hl = ip_hdr_in->ip_hl;
            ip_hdr_out->ip_tos = ip_hdr_in->ip_tos;
            ip_hdr_out->ip_len = ip_hdr_in->ip_len;
            ip_hdr_out->ip_id = ip_hdr_in->ip_id;
            ip_hdr_out->ip_off = 0;
            ip_hdr_out->ip_ttl = 255;
            
            ip_hdr_out->ip_p = IPPROTO_ICMP;
            ip_hdr_out->ip_sum = 0;
            ip_hdr_out->ip_src.s_addr = ip_hdr_in->ip_dst.s_addr;
            ip_hdr_out->ip_dst.s_addr = ip_hdr_in->ip_src.s_addr;

            icmp_hdr_out->icmp_type = ICMP_ECHOREPLY;
            icmp_hdr_out->icmp_code = 0;
            icmp_hdr_out->icmp_id = icmp_hdr_in->icmp_id;
            icmp_hdr_out->icmp_seq = icmp_hdr_in->icmp_seq + 1;

            ip_len = ntohs (ip_hdr_in->ip_len);
            icmp_len = ip_len - sizeof (struct iphdr);
            icmp_data_in_len = ntohs(ip_hdr_in->ip_len) - sizeof(struct ip) - sizeof(struct icmphdr);
            
            memset(payload, 0, sizeof(payload));
            memcpy(payload, icmp_hdr_in->icmp_data, icmp_data_in_len);
            if (payload[0] == '.' && icmp_data_in_len == 1){
               break;
            }
            // Save the payload 
            write(fd, payload, icmp_data_in_len);
            // reset the buffer
            buf_incoming[0] = '\0';

         }
      }
  }
   close(fd);
   close(sock_eth);
   close(sock_icmp);
   return 0;
}
