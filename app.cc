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
	
	
	state Menu_Print:
		ser_outf(Menu_Print, "\r\nGroup%d Device #%d (%d/%d records)", group_ID, node_ID, stored_records, max_records);
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

