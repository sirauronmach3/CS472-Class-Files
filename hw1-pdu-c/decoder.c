#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include "packet.h"
#include "nethelper.h"
#include "decoder.h"

// This is where you will be putting your captured network frames for testing.
// Before you do your own, please test with the ones that I provided as samples:
#include "testframes.h"

// You can update this array as you add and remove test cases, you can
// also comment out all but one of them to isolate your testing. This
// allows us to loop over all of the test cases.  Note MAKE_PACKET creates
// a test_packet_t element for each sample, this allows us to get and use
// the packet length, which will be helpful later.
test_packet_t TEST_CASES[] = {
    MAKE_PACKET(raw_packet_icmp_frame198),
    MAKE_PACKET(raw_packet_icmp_frame362),
    MAKE_PACKET(raw_packet_arp_frame78)};

// !!!!!!!!!!!!!!!!!!!!! WHAT YOU NEED TO DO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// Search the code for TODO:, each one of these describes a place where
// you need to write code.  This scaffold should compile as is.  Make sure
// you delete the TODO: documentation in your implementation and provide
// some documentation on what you actually accomplished.

int main(int argc, char **argv)
{
    // This code is here as a refresher on how to figure out how
    // many elements are in a statically defined C array. Note
    // that sizeof(TEST_CASES) is not 3, its the total number of
    // bytes.  On my machine it comes back with 48, because each
    // element is of type test_packet_t which on my machine is 16 bytes.
    // Thus, with the scaffold I am providing 48/16 = 3, which is
    // the correct size.
    int num_test_cases = sizeof(TEST_CASES) / sizeof(test_packet_t);

    printf("STARTING...");
    for (int i = 0; i < num_test_cases; i++)
    {
        printf("\n--------------------------------------------------\n");
        printf("TESTING A NEW PACKET\n");
        printf("--------------------------------------------------\n");
        test_packet_t test_case = TEST_CASES[i];

        decode_raw_packet(test_case.raw_packet, test_case.packet_len);
    }

    printf("\nDONE\n");
}

void decode_raw_packet(uint8_t *packet, uint64_t packet_len)
{

    printf("Packet length = %ld bytes\n", packet_len);

    // Everything we are doing starts with the ethernet PDU at the
    // front.  The below code projects an ethernet_pdu structure
    // POINTER onto the front of the buffer so we can decode it.
    struct ether_pdu *p = (struct ether_pdu *)packet;
    uint16_t ft = ntohs(p->frame_type);

    printf("Detected raw frame type from ethernet header: 0x%x\n", ft);

    switch (ft)
    {
    case ARP_PTYPE:
        printf("Packet type = ARP\n");

        // Lets process the ARP packet, convert all of the network byte order
        // fields to host machine byte order
        arp_packet_t *arp = process_arp(packet);

        // Print the arp packet
        print_arp(arp);
        break;
    case IP4_PTYPE:
        printf("Frame type = IPv4, now lets check for ICMP...\n");

        // We know its IP, so lets type the raw packet as an IP packet
        ip_packet_t *ip = (ip_packet_t *)packet;

        // Now check the IP packet to see if its payload is an ICMP packet
        bool isICMP = check_ip_for_icmp(ip);
        if (!isICMP)
        {
            printf("ERROR: IP Packet is not ICMP\n");
            break;
        }

        // Now lets process the basic icmp packet, convert the network byte order
        // fields to host byte order
        icmp_packet_t *icmp = process_icmp(ip);

        // Now lets look deeper and see if the icmp packet is actually an
        // ICMP ECHO packet?
        bool is_echo = is_icmp_echo(icmp);
        if (!is_echo)
        {
            printf("ERROR: We have an ICMP packet, but it is not of type echo\n");
            break;
        }

        // Now lets process the icmp_packet as an icmp_echo_packet, again processing
        // the network byte order fields
        icmp_echo_packet_t *icmp_echo_packet = process_icmp_echo(icmp);

        // The ICMP packet now has its network byte order fields
        // adjusted, lets print it
        print_icmp_echo(icmp_echo_packet);

        break;
    default:
        printf("UNKNOWN Frame type?\n");
    }
}

/********************************************************************************/
/*                       ARP PROTOCOL HANDLERS                                  */
/********************************************************************************/

/*
 *  This function takes a raw_packet that has already been verified to be an ARP
 *  packet.  It typecasts the raw_packet into an arp_packet_t *, and then
 *  converts all of the network byte order fields into host byte order.
 */
arp_packet_t *process_arp(raw_packet_t raw_packet)
{
    arp_packet_t *arp = (arp_packet_t *)raw_packet;

    arp->arp_hdr.htype = ntohs(arp->arp_hdr.htype);
    arp->arp_hdr.ptype = ntohs(arp->arp_hdr.ptype);
    arp->arp_hdr.op = ntohs(arp->arp_hdr.op);

    return arp;
}

/*
 *  This function takes an arp packet and just pretty-prints it to stdout using
 *  printf.  It decodes and indicates in the output if the request was an
 *  ARP_REQUEST or an ARP_RESPONSE
 */
void print_arp(arp_packet_t *arp)
{
    char string[18];
    /*
    Packet length = 60 bytes
    Detected raw frame type from ethernet header: 0x806
    Packet type = ARP
    ARP PACKET DETAILS
         htype:     0x0001
         ptype:     0x0800
         hlen:      6
         plen:      4
         op:        1 (ARP REQUEST)
         spa:       192.168.50.1
         sha:       a0:36:bc:62:ed:50
         tpa:       192.168.50.99
         tha:       00:00:00:00:00:00
     */
    fprintf(stdout, "ARP PACKET DETAILS\n");
    fprintf(stdout, "     htype:     0x%04x\n", arp->arp_hdr.htype);
    fprintf(stdout, "     ptype:     0x%04x\n", arp->arp_hdr.ptype);
    fprintf(stdout, "     hlen:      %d\n", arp->arp_hdr.hlen);
    fprintf(stdout, "     plen:      %d\n", arp->arp_hdr.plen);
    fprintf(stdout, "     op:        %d (%s)\n", arp->arp_hdr.op,
            (arp->arp_hdr.op == ARP_REQ_OP) ? "ARP REQUEST" : "ARP RESPONSE");
    if (ip_toStr(arp->arp_hdr.spa, string, 16) == 1)
    {
        fprintf(stdout, "     spa:       %s\n", string);
    }
    if (mac_toStr(arp->arp_hdr.sha, string, 18) == 1)
    {
        fprintf(stdout, "     sha:       %s\n", string);
    }
    if (ip_toStr(arp->arp_hdr.tpa, string, 16) == 1)
    {
        fprintf(stdout, "     tpa:       %s\n", string);
    }
    if (mac_toStr(arp->arp_hdr.tha, string, 18) == 1)
    {
        fprintf(stdout, "     tha:       %s\n", string);
    }
}

/********************************************************************************/
/*                       ICMP PROTOCOL HANDLERS                                  */
/********************************************************************************/

/*
 *  This function takes an ip packet and then inspects its internal fields to
 *  see if the IP packet is managing an underlying ICMP packet.  If so, return
 *  true, if not return false.  You need to see if the "protocol" field in the
 *  IP PDU is set to ICMP_PTYPE to do this.
 */
bool check_ip_for_icmp(ip_packet_t *ip)
{
    if (ip->ip_hdr.protocol == ICMP_PTYPE)
    {
        return true;
    }
    return false;
}

/*
 *  This function takes an IP packet and converts it into an icmp packet. Note
 *  that it is assumed that we already checked if the IP packet is encapsulating
 *  an ICMP packet.  So we need to type convert it from (ip_packet_t *) to
 *  (icmp_packet *).  There are some that need to be converted from
 *  network to host byte order.
 */
icmp_packet_t *process_icmp(ip_packet_t *ip)
{
    icmp_packet_t *icmp = (icmp_packet_t *)ip;
    icmp->icmp_hdr.checksum = ntohs(icmp->icmp_hdr.checksum);

    return icmp;
}

/*
 *  This function takes a known ICMP packet, and checks if its of type ECHO. We do
 *  this by checking the "type" field in the icmp_hdr and evaluating if its equal to
 *  ICMP_ECHO_REQUEST or ICMP_ECHO_RESPONSE.  If true, we return true. If not, its
 *  still ICMP but not of type ICMP_ECHO.
 */
bool is_icmp_echo(icmp_packet_t *icmp)
{
    if (icmp->icmp_hdr.type == ICMP_ECHO_REQUEST || icmp->icmp_hdr.type == ICMP_ECHO_RESPONSE)
    {
        return true;
    }
    return false;
}

/*
 *  This function takes a known ICMP packet, that has already been checked to be
 *  of type ECHO and converts it to an (icmp_echo_packet_t).  Like in the other
 *  cases this is simply a type converstion, but there are also a few fields to
 *  convert from network to host byte order.
 */
icmp_echo_packet_t *process_icmp_echo(icmp_packet_t *icmp)
{
    icmp_echo_packet_t *icmp_echo = (icmp_echo_packet_t *)icmp;

    icmp_echo->icmp_echo_hdr.id = ntohs(icmp_echo->icmp_echo_hdr.id);
    icmp_echo->icmp_echo_hdr.sequence = ntohs(icmp_echo->icmp_echo_hdr.sequence);
    icmp_echo->icmp_echo_hdr.timestamp = ntohl(icmp_echo->icmp_echo_hdr.timestamp);
    icmp_echo->icmp_echo_hdr.timestamp_ms = ntohl(icmp_echo->icmp_echo_hdr.timestamp_ms);

    return icmp_echo;
}

/*
 *  This function pretty prints the icmp_packet.  After it prints the header aka PDU
 *  it calls print_icmp_payload to print out the echo packet variable data.  To do
 *  this it needs to calculate the length of the "payload" field.  To make things
 *  easier for you to call print_icmp_payload you can use a macro I provided.  Thus...
 *
 *  uint16_t payload_size = ICMP_Payload_Size(icmp_packet);
 *
 *  gives the size of the payload buffer.
 */
void print_icmp_echo(icmp_echo_packet_t *icmp_packet)
{
    /*
    Packet length = 98 bytes
    Detected raw frame type from ethernet header: 0x800
    Frame type = IPv4, now lets check for ICMP...
    ICMP Type 8
    ICMP PACKET DETAILS
         type:      0x08
         checksum:  0x7bda
         id:        0x4859
         sequence:  0x0000
         timestamp: 0x650e01eee1cc
         payload:   48 bytes
         ECHO Timestamp: TS = 2023-09-22 21:06:54.57804
     */

    fprintf(stdout, "ICMP PACKET DETAILS\n");
    fprintf(stdout, "     type:      0x%02x\n", icmp_packet->icmp_echo_hdr.icmp_hdr.type);
    fprintf(stdout, "     checksum:  0x%04x\n", icmp_packet->icmp_echo_hdr.icmp_hdr.checksum);
    fprintf(stdout, "     id:        0x%04x\n", icmp_packet->icmp_echo_hdr.id);
    fprintf(stdout, "     sequence:  0x%04x\n", icmp_packet->icmp_echo_hdr.sequence);
    fprintf(stdout, "     timestamp: 0x%08lx%08lx\n", icmp_packet->icmp_echo_hdr.timestamp, icmp_packet->icmp_echo_hdr.timestamp_ms);
    uint16_t payload_size = ICMP_Payload_Size(icmp_packet);
    fprintf(stdout, "     payload:   %d bytes\n", payload_size);
    fprintf(stdout, "     ECHO Timestamp: %s\n", get_ts_formatted(icmp_packet->icmp_echo_hdr.timestamp, icmp_packet->icmp_echo_hdr.timestamp_ms));

    print_icmp_payload(icmp_packet->icmp_payload, payload_size);
}

/*
 *  This function pretty prints the icmp_echo_packet payload.  You can be
 *  creative here, but try to make it look nice.  Below is an example of
 *  how I printed it.  You basically do this by looping trough each
 *  byte in the payload.  Below, I set the line length to 16.  So, as we
 *  loop through the array with an index (lets call this "i"), with a
 *  line_len = 16 we do the following:
 *
 *  if (i % line_length) == 0 then we have a new line, write offset which is
 *      the loop index i
 *
 *  we next write the element at buffer[i]
 *
 *  if (i % line_lenght) == (line_lenght - 1) then we write a newline "\n"
 *
 *  You dont have to make it look exactly like I made my solution shown below
 *  but it should look nice :-)
 *
 * PAYLOAD
 *
 * OFFSET | CONTENTS
 * -------------------------------------------------------
 * 0x0000 | 0x08  0x09  0x0a  0x0b  0x0c  0x0d  0x0e  0x0f
 * 0x0008 | 0x10  0x11  0x12  0x13  0x14  0x15  0x16  0x17
 * 0x0010 | 0x18  0x19  0x1a  0x1b  0x1c  0x1d  0x1e  0x1f
 * 0x0018 | 0x20  0x21  0x22  0x23  0x24  0x25  0x26  0x27
 * 0x0020 | 0x28  0x29  0x2a  0x2b  0x2c  0x2d  0x2e  0x2f
 * 0x0028 | 0x30  0x31  0x32  0x33  0x34  0x35  0x36  0x37
 */
void print_icmp_payload(uint8_t *payload, uint16_t payload_size)
{
    fprintf(stdout, "PAYLOAD\n\n");
    fprintf(stdout, "OFFSET | CONTENTS\n");
    fprintf(stdout, "-------------------------------------------------------\n");
    for (int i = 0; i < payload_size; i++)
    {
        if (i % 8 == 0)
        {
            fprintf(stdout, "0x%04x | ", i);
        }
        fprintf(stdout, "0x%02x  ", payload[i]);
        if (i % 8 == 7)
        {
            fprintf(stdout, "\n");
        }
    }
    if (payload_size % 8 != 0)
    {
        fprintf(stdout, "\n");
    }
}
