#include <pcap.h>
#include <stdbool.h>
#include <stdio.h>
#define ETHER_ADDR_LEN 6
#define LIBNET_LIL_ENDIAN 1

struct libnet_ipv4_hdr
{
#if (LIBNET_LIL_ENDIAN)
	u_int8_t ip_hl : 4, /* header length */
		ip_v : 4;		/* version */
#endif
#if (LIBNET_BIG_ENDIAN)
	u_int8_t ip_v : 4, /* version */
		ip_hl : 4;	   /* header length */
#endif
	u_int8_t ip_tos; /* type of service */
#ifndef IPTOS_LOWDELAY
#define IPTOS_LOWDELAY 0x10
#endif
#ifndef IPTOS_THROUGHPUT
#define IPTOS_THROUGHPUT 0x08
#endif
#ifndef IPTOS_RELIABILITY
#define IPTOS_RELIABILITY 0x04
#endif
#ifndef IPTOS_LOWCOST
#define IPTOS_LOWCOST 0x02
#endif
	u_int16_t ip_len; /* total length */
	u_int16_t ip_id;  /* identification */
	u_int16_t ip_off;
#ifndef IP_RF
#define IP_RF 0x8000 /* reserved fragment flag */
#endif
#ifndef IP_DF
#define IP_DF 0x4000 /* dont fragment flag */
#endif
#ifndef IP_MF
#define IP_MF 0x2000 /* more fragments flag */
#endif
#ifndef IP_OFFMASK
#define IP_OFFMASK 0x1fff /* mask for fragmenting bits */
#endif
	u_int8_t ip_ttl;			   /* time to live */
	u_int8_t ip_p;				   /* protocol */
	u_int16_t ip_sum;			   /* checksum */
	struct in_addr ip_src, ip_dst; /* source and dest address */
};

struct libnet_tcp_hdr
{
	u_int16_t th_sport; /* source port */
	u_int16_t th_dport; /* destination port */
	u_int32_t th_seq;	/* sequence number */
	u_int32_t th_ack;	/* acknowledgement number */
#if (LIBNET_LIL_ENDIAN)
	u_int8_t th_x2 : 4, /* (unused) */
		th_off : 4;		/* data offset */
#endif
#if (LIBNET_BIG_ENDIAN)
	u_int8_t th_off : 4, /* data offset */
		th_x2 : 4;		 /* (unused) */

#endif
	u_int8_t th_flags; /* control flags */
#ifndef TH_FIN
#define TH_FIN 0x01 /* finished send data */
#endif
#ifndef TH_SYN
#define TH_SYN 0x02 /* synchronize sequence numbers */
#endif
#ifndef TH_RST
#define TH_RST 0x04 /* reset the connection */
#endif
#ifndef TH_PUSH
#define TH_PUSH 0x08 /* push data to the app layer */
#endif
#ifndef TH_ACK
#define TH_ACK 0x10 /* acknowledge */
#endif
#ifndef TH_URG
#define TH_URG 0x20 /* urgent! */
#endif
#ifndef TH_ECE
#define TH_ECE 0x40
#endif
#ifndef TH_CWR
#define TH_CWR 0x80
#endif
	u_int16_t th_win; /* window */
	u_int16_t th_sum; /* checksum */
	u_int16_t th_urp; /* urgent pointer */
};
struct libnet_ethernet_hdr
{
	u_int8_t ether_dhost[ETHER_ADDR_LEN]; /* destination ethernet address */
	u_int8_t ether_shost[ETHER_ADDR_LEN]; /* source ethernet address */
	u_int16_t ether_type;				  /* protocol */
};

typedef struct
{
	char *dev_;
} Param;

Param param = {
	.dev_ = NULL};

void usage()
{
	printf("syntax: pcap-test <interface>\n");
	printf("sample: pcap-test wlan0\n");
}

bool parse(Param *param, int argc, char *argv[])
{
	if (argc != 2)
	{
		usage();
		return false;
	}
	param->dev_ = argv[1];
	return true;
}
void printMac(u_int8_t *m)
{
	printf("%02x:%02x:%02x:%02x:%02x:%02x", m[0], m[1], m[2], m[3], m[4], m[5]);
}
int main(int argc, char *argv[])
{
	if (!parse(&param, argc, argv))
		return -1;

	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_t *pcap = pcap_open_live(param.dev_, BUFSIZ, 1, 1000, errbuf);
	if (pcap == NULL)
	{
		fprintf(stderr, "pcap_open_live(%s) return null - %s\n", param.dev_, errbuf);
		return -1;
	}

	while (true)
	{
		struct pcap_pkthdr *header;
		const unsigned char *packet;
		int res = pcap_next_ex(pcap, &header, &packet);
		if (res == 0)
			continue;
		if (res == PCAP_ERROR || res == PCAP_ERROR_BREAK)
		{
			printf("pcap_next_ex return %d(%s)\n", res, pcap_geterr(pcap));
			break;
		}
		printf("%u bytes captured\n", header->caplen);
		struct libnet_ethernet_hdr *eth_hdr = (struct libnet_ethernet_hdr *)packet;
		printf("source mac ");
		printMac(eth_hdr->ether_shost);
		printf("\tDestination mac ");
		printMac(eth_hdr->ether_dhost);
		printf("\n");
		if (ntohs(eth_hdr->ether_type) != 0x0800)
			continue;
		unsigned long ether_size = sizeof(struct libnet_ethernet_hdr);
		struct libnet_ipv4_hdr *ipv4_hdr = (struct libnet_ipv4_hdr *)(ether_size + packet);

		printf("version : %d\n",ipv4_hdr->ip_v & 0xF);
		printf("Header Length: %d bytes (%d)\n",ipv4_hdr->ip_hl*4, ipv4_hdr->ip_hl);
		printf("Differentiated Services Field : 0x%02x\n",ipv4_hdr->ip_tos & 0xF);
		printf("Total Length : %d\n",ntohs(ipv4_hdr->ip_len));
		printf("Identification : 0x%04x (%d)\n",ipv4_hdr->ip_id,ipv4_hdr->ip_id);
		struct in_addr sip = ipv4_hdr->ip_src;
		u_int32_t ip = ntohl(sip.s_addr);
		printf("source ip : ");
		
		for (int i = 3; i >= 0; i--) {
        	printf("%d", (ip >> (i * 8)) & 0xFF);
			if (i != 0) printf(".");
   		}
		struct in_addr dip = ipv4_hdr->ip_dst;
		ip = ntohl(dip.s_addr);
		
		printf("\tDestination ip : ");
		for (int i = 3; i >= 0; i--) {
        	printf("%d", (ip >> (i * 8)) & 0xFF);
			if (i != 0) printf(".");
   		}
    		
		printf("\n");
		unsigned long ipv4_size = sizeof(struct libnet_ipv4_hdr);
		struct libnet_tcp_hdr *tcp_hdr = (struct libnet_tcp_hdr *)(ether_size + ipv4_size + packet);
		printf("source port : %d",ntohs(tcp_hdr->th_sport));
		printf("\tDestination port : %d\n",ntohs(tcp_hdr->th_dport));
		printf("\n");
		unsigned long tcp_hdr_size = sizeof(struct libnet_tcp_hdr);

        	unsigned long headers_size = header->caplen -ntohs(ipv4_hdr->ip_len) +
		ether_size + 
		ipv4_size + 
		tcp_hdr->th_off*4;

        	const unsigned char *data = packet + headers_size;
		printf("Data (%ld bytes) : ",headers_size);
    		for (int i = 0; i < headers_size; i++) 
        		printf("%02x ", data[i]);
    		printf("\n");
	}
	pcap_close(pcap);
}
