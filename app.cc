// assignment 2 

#include "sysio.h"
#include "ser.h"
#include "serf.h"
#include "tcv.h"

#include "phys_cc1350.h"
#include "plug_null.h"

#define MAX_PACKET_LENGTH 250
#define MAX_MES_LEN 14

int sfd = -1; //session descriptor 

int node_ID;
int group_ID;
int stored_records;
int max_records;
byte delete_request_num;
byte delete_destination_id;
byte delete_response_received;
byte delete_response_status;
//record structure
struct Record{
	byte used; //0 for empty, 1 for used
	byte owner_id; //owner id of the record
	int timestamp; //stores the timestamp of the record when it was made
	char record[20]; //stores the record data of up to 20 characters
};
//database storage for up to 40 records
struct Record database[40];

fsm Delete{
	address packet;
	char *p;
	int destination_id;
	int record_index;
	byte request_num;
	//prompts the user for the destination node ID to delete
	state Ask_Destination_ID:
		ser_out(Ask_Destination_ID, "Destination node ID: ");
		ser_inf(Ask_Destination_ID, "%d", &destination_id);
	//prompts the user of the record index to delete
	state Ask_Record_Index:
		ser_out(Ask_Record_Index, "Enter record index: ");
		ser_inf(Ask_Record_Index, "%d", &record_index);
	//prepares the delete request packet
	state Send_delete_request:
		delete_request_num = (byte)rnd();
		delete_destination_id = (byte)destination_id;
		delete_response_received = 0;

		packet = tcv_wnp(Send_delete_request, sfd, 8);

		packet[0] = 0;
		p = (char *)(packet +1);

		*p++ = (group_ID >> 8) & 0xFF;
		*p++ = group_ID & 0xFF;
		*p++ = 3;
		*p++ = delete_request_num;
		*p++ = node_ID;
		*p++ = delete_destination_id;
		*p++ = record_index;
		*p++ = 0;

		tcv_endp(packet);
	
	
	state Wait_Response:

		if (delete_response_received){
			proceed Print_Result;
		}
		delay(3000, Timeout);
		proceed Wait_Response;
	state Timeout:
		if (delete_response_received){
			proceed Print_Result;
		}

		ser_out(Timeout, "\r\nFailed to reach the destination");
		finish;
	state Print_Result:
		if (delete_response_status == 0x01){
			ser_out(Timeout, "\r\nRecord deleted");
		}
		else{
			ser_outf(Timeout, "\r\nThe record does not exit on node %d", delete_destination_id);
		}
		finish;
}
fsm Delete_Receiver{
	address packet;
	address response;
	char *p;
	char *q;
	int pkt_group;

	byte request_num;
	byte type;
	byte req_no;
	byte sender_id;
	byte receiver_id;
	byte record_index;
	byte status;
	state wait_packet:
		packet = tcv_rnp(wait_packet, sfd);
		p = (char *)(packet + 1);
		pkt_group = ((byte)p[0] <<8) | (byte)p[1];
		type = p[2];
		req_no = p[3];
		sender_id = p[4];
		receiver_id = p[5];
		if(type == 5){
			status = p[6];
			if(receiver_id == node_ID && sender_id == delete_destination_id && req_no == delete_request_num){
				delete_response_received = 1;
				delete_response_status = status;
			}
			tcv_endp(packet);
			proceed wait_packet;
		}
		if (type != 3){
			tcv_endp(packet);
			proceed wait_packet;
		}
		record_index = p[6];
		proceed handle_delete;

	state handle_delete:
		if (pkt_group != group_ID || receiver_id != node_ID){
			tcv_endp(packet);
			proceed wait_packet;
		}
		if (record_index >= max_records || database[record_index].used == 0){
			status = 0x03;
		}
		else{
			database[record_index].used = 0;
			database[record_index].owner_id = 0;
			database[record_index].timestamp = 0;
			database[record_index].record[0] = '\0';
			stored_records--;
			status = 0x01;
		}
		tcv_endp(packet);
		proceed send_response;

	state send_response:
		response = tcv_wnp(send_response, sfd, 28);

		response[0] = 0;
		q = (char *)(response + 1);
		
		*q++ = (group_ID >> 8) & 0xFF;
		*q++ = group_ID & 0xFF;
		*q++ = 5;
		*q++ = req_no;
		*q++ = node_ID;
		*q++ = sender_id;
		*q++ = status;
		*q++ = 0;

		memset(q, 0, RECORD_LEN);
		tcv_endp(response);
		proceed wait_packet;
}
fsm root
{
	char choice;
	int temp;
	
	state Init:
		phys_cc1350 (0, MAX_PACKET_LENGTH);
		
		tcv_plug (0, &plug_null);
		sfd = tcv_open(WNONE, 0, 0);
		tcv_control (sfd, PHYSOPT_ON, NULL);
		
		if (sfd < 0)
		{
			diag("Cannot open tcv interface");
			halt();
		}
		runfsm Delete_Receiver;
	
	
	state Menu_Print:
		ser_outf(Menu_Print, "\r\nGroup%d Device #%d (%d/%d records)", group_ID, node_ID, stored_records, 40);
		ser_out(Menu_Print, "(G)roup ID\r\n(N)ew device ID\r\n(F)ind neighbour\r\n(R)etrieve record from neighbour\r\n(S)how local records\r\nR(e)set local storage\r\n\r\nSelection: ");
	
	state Read_Choice:
		// read the user input 
		ser_inf(Read_Choice, "%d", &temp);
		
		// if "G" or "g", state Ask_Group_ID
		if ((choice == 'G') || (choice == 'g'))
		{
			proceed Ask_Group_ID;	
		}
		
		// if "N" or "n", state Ask_Node
		else if ((choice == 'N') || (choice == 'n'))
		{
			proceed Ask_Node_ID;	
		}
		
		// if "F", run Find protocol
		else if ((choice == 'F') || (choice == 'f'))
		{
			runfsm Find; // change to match fsm name if different
		}
		
		// if "C, run Create protocol
		else if ((choice == 'C') || (choice == 'c'))
		{
			runfsm Create;
		}
		
		// if "D", run Delete protocol
		else if ((choice == 'D') || (choice == 'd'))
		{
			runfsm Delete;
		}
		
		// if "R", run the Retrieve protocol
		else if ((choice == 'R') || (choice == 'r'))
		{
			runfsm Retrieve;
		}
		
		// if "S", go to Show_LocalDB state
		else if ((choice == 'S') || (choice == 's'))
		{
			proceed Show_LocalDB;
		}
		// if "E", go to Reset_DB state
		else if ((choice == 'E') || (choice == 'e'))
		{
			proceed Reset_DB;
		}
		
		// any other input, show menu again
		else
		{
			proceed Menu_Print;
		}
		
		
		
		
	state Ask_Group_ID:
		// ask for group ID 
		 
		ser_out(Ask_Group_ID, "Enter Group ID: ");
		
		// update node group ID 
		
		
		// show the menu again 
		proceed Menu_Print;
		
	
	state Ask_Node_ID:
		// ask for Node ID 
		ser_out(Ask_Node_ID, "Enter Node ID: ");
		
		//update node ID
		
		
		// show the menu 
		proceed Menu_Print;
		
		
	state Read_Node:
		// reads from the specified node given by user
		
	state Show_LocalDB:
		// user chose "S" or "s", 
		// format: index    Time Stamp     owner ID     Record Data
	
	
	state Reset_DB:
		// user chose "E" or "e", delete all the nodes 
}

