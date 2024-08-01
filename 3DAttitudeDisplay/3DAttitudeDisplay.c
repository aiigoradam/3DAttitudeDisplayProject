//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include <rs232.h>
#include "inifile.h"
#include "cviogl.h" 
#include "3DAttitudeDisplay.h"
#include "locatecom.h"

//----------------------------------------------------------------------------
// Constants
//----------------------------------------------------------------------------
#define INI_FILE_NAME   		"config.ini" 
#define TIME_FORMATSTRING		"[%H:%M:%S]"
#define MAINPANEL_HEIGHT 		620
#define	MAINPANEL_WIDTH  		736
#define PACKET_SIZE 			(NUM_ELEMENTS * sizeof(float)) + 1 //in bytes
#define NUM_ELEMENTS 			3    // Number of elements received from the controller
#define FLAG 					10   // Linefeed character '\n'

//----------------------------------------------------------------------------
// Global Variables
//----------------------------------------------------------------------------
int 				mainPanel; 
int					OGLControlID;
int					configHandle;
int 				textboxWidth;
int					baudRate;
int					parity;
int					dataBits;
int				 	stopBits;
int volatile 		portNumber; 
int volatile 		stop;
int					scrolled;
int 				currentPort;
float 				pitch;
float 				yaw;
float 				roll;
float 				barometer; 
float 				dataVector[NUM_ELEMENTS]; 
char 				packet[PACKET_SIZE];
IniText 			iniHandle;
FILE 				*logFile;  

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------
static int CVICALLBACK ThreadFunction (void *functionData); 
void CVICALLBACK ComCallback (int MyCounter, int eventMask, void *callbackData);
void CVICALLBACK DeferredCallbackFunction (void *callbackData);
void InitOGLControl(void);   
void FindComPorts();
void LoadConfiguration(); 
void SaveConfiguration();
void LogMessage(const char *format, ...);
int IntCompareWrapper(const void *item1, const void *item2);    

//----------------------------------------------------------------------------
//  Main
//----------------------------------------------------------------------------
int main (int argc, char *argv[])
{
    if (InitCVIRTE (0, argv, 0) == 0)   /* Needed if linking in external compiler; harmless otherwise */
        return -1;  /* out of memory */

    // Load the main program panel
    if ((mainPanel = LoadPanel (0, "3DAttitudeDisplay.uir", PANEL)) < 0)
		return -1;
	
	// Load the config panel   
	if ((configHandle = LoadPanel (0, "3DAttitudeDisplay.uir", CONFIG)) < 0)
		return -1;

	// Set appropriate panel size
	GetCtrlAttribute (mainPanel, PANEL_TEXTBOX, ATTR_WIDTH, &textboxWidth); // Needed to adjust panel size
	SetPanelSize (mainPanel, MAINPANEL_HEIGHT, MAINPANEL_WIDTH );
	
	// Open the log file in write mode
	logFile = fopen("logfile.txt", "w");
	LogMessage("Log file opened successfully.");

	// Find com ports and display on ring menu
	FindComPorts();
	LogMessage("Ports loaded successfully.");
	
	// Load configuration from INI file
	LoadConfiguration();
	LogMessage("Configuration loaded successfully.");

	// Display the loaded port number
	SetCtrlVal (mainPanel, PANEL_RING, portNumber); 
	currentPort = portNumber; // Retain the loaded port number

	// Convert the CVI picture control to an OGL control
	OGLControlID = OGLConvertCtrl (mainPanel, PANEL_3D_IMAGE);

	// Initialize the OGL control
	InitOGLControl();

	// Display plot
	OGLRefreshGraph(mainPanel, OGLControlID);

	// Display the main Panel
	DisplayPanel(mainPanel);

	LogMessage("Entering UI loop.");
	
	// Enter the UI loop
	RunUserInterface();
	
	// Hide the panel and discard the OGL control along with the panel
	HidePanel (mainPanel);
	OGLDiscardCtrl(mainPanel,OGLControlID);
	DiscardPanel (mainPanel);
	return 0;
}

int CVICALLBACK PanelCallback(int panel, int event, void *callbackData, int eventData1, int eventData2)
{
	int disconnected;
	switch (event)
	{
		case EVENT_CLOSE:
			GetCtrlAttribute (mainPanel, PANEL_DISCONNECT, ATTR_DIMMED, &disconnected);
			if(!disconnected)
				DisconnectButtonCallback(0,0,EVENT_COMMIT,0,0,0);
			
			LogMessage("Closing program...");  

			// Save configuration to INI file before closing
			SaveConfiguration();
			
			LogMessage("Configuration saved successfully.");
			
			LogMessage("Discarding resources and exiting UI loop.");  
			// Close the log file      
			fclose(logFile); 
			
			QuitUserInterface(0);
			break;
	}
	return 0;
}

int CVICALLBACK DisconnectButtonCallback (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			LogMessage("====== Stopping ======");
			// disable Disconnect button 
			SetCtrlAttribute (mainPanel, PANEL_DISCONNECT, ATTR_DIMMED, 1);
			
			stop = 1;
			ProcessSystemEvents ();
			Delay(0.2); // Wait until ComCallback has finished

			LogMessage("Disconnecting from COM port %d...", portNumber);
			CloseCom(portNumber);
			LogMessage("Disconnected from COM port %d.", portNumber);
			
			// Enable Connect and Ring controls
			SetCtrlAttribute (mainPanel, PANEL_CONNECT, ATTR_DIMMED, 0);
			SetCtrlAttribute (mainPanel, PANEL_RING, ATTR_DIMMED, 0);
			break;
	}
	return 0;
}


/*---------------------------------------------------------------------------*/
/* Connect to the COM port in a separate thread 					         */
/*---------------------------------------------------------------------------*/
int CVICALLBACK ConnectButtonCallback (int panel, int control, int event,
								void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			LogMessage("====== Running ======"); 
			
			stop = 0;
			
			// Disable Connect and Ring controls   
			SetCtrlAttribute (mainPanel, PANEL_CONNECT, ATTR_DIMMED, 1);
			SetCtrlAttribute (mainPanel, PANEL_RING, ATTR_DIMMED, 1);
		
			// Get the port number from the Ring control
			GetCtrlVal (mainPanel, PANEL_RING, &portNumber);

			// Schedule the thread function to receive data
			CmtScheduleThreadPoolFunction(DEFAULT_THREAD_POOL_HANDLE, ThreadFunction, 0, 0);
			break;
	}
	return 0;
}

static int CVICALLBACK ThreadFunction (void *functionData)
{
	// Open the COM port
	LogMessage("Connecting to COM port %d...", portNumber);
	SetBreakOnLibraryErrors (0);
	int RS232Error = OpenComConfig (portNumber, "", baudRate, parity, dataBits, stopBits, 4096, 4096);
	SetBreakOnLibraryErrors (1);

	if (RS232Error >= 0)
	{
		LogMessage("Connected to COM port %d.", portNumber);   
		char parityLabel[16]; 
		GetLabelFromIndex (configHandle, CONFIG_PARITY, parity, parityLabel);
		LogMessage("BaudRate: %d, Parity: %s, DataBits: %d, StopBits: %d", baudRate, parityLabel, dataBits, stopBits);
		FlushInQ (portNumber);
		
		// The callback function is called whenever FLAG occurs in the queue
		InstallComCallback (portNumber, LWRS_RXFLAG, 0, FLAG, ComCallback, 0);
		
		SetCtrlAttribute (mainPanel, PANEL_DISCONNECT, ATTR_DIMMED, 0);
		
		// Set DTR ON to establish connection
	//	ComSetEscape (portNumber, SETDTR);

		// Send break signal to start transmission
		ComBreak (portNumber, 25);
	}
	else // Display the error
	{
		LogMessage("[ERROR] %s", GetRS232ErrorString (RS232Error));
		SetCtrlAttribute (mainPanel, PANEL_CONNECT, ATTR_DIMMED, 0);
		SetCtrlAttribute (mainPanel, PANEL_RING, ATTR_DIMMED, 0);
	}
	return 0;
}


void CVICALLBACK ComCallback (int portNumber, int eventMask, void *callbackData)
{
	if(stop) 
	{
		// CloseCom uninstalls the ComCallback and closes the port
		CloseCom(portNumber);
		return;
	}
	
	// Display the input queue buffer size in bytes
	SetCtrlVal (mainPanel, PANEL_INQUE, GetInQLen(portNumber));

	LogMessage("*Start reading*");

	// Read from the com port 
	int bytesRead = ComRd(portNumber, packet, PACKET_SIZE);

	LogMessage("*Reading complete*");

	// Check if the packet size is correct  
	if (bytesRead != PACKET_SIZE) 
	{
		LogMessage("[ERROR] Com read PACKET_SIZE mismatch");
		return; 
	}
	
	// Check if the last byte is the flag
	if (packet[PACKET_SIZE - 1] != FLAG)
	{
		LogMessage("[ERROR] FLAG mismatch");
		SetBreakOnLibraryErrors (0);  
		FlushInQ (portNumber);
		SetBreakOnLibraryErrors (1);  
		return;
	}

	// Extract data from the received packet
	memcpy(dataVector, packet, PACKET_SIZE-1);

	roll  = dataVector[0];
	pitch = dataVector[1];
	yaw   = dataVector[2];

	// Update UI control elements
	SetCtrlVal (mainPanel, PANEL_ROLL,  roll);
	SetCtrlVal (mainPanel, PANEL_PITCH, pitch);
	SetCtrlVal (mainPanel, PANEL_YAW,   yaw);
	
	// Tell the main thread to refresh the OGL graph
	PostDeferredCall (DeferredCallbackFunction, 0);

}

void CVICALLBACK DeferredCallbackFunction (void *callbackData)
{
	// Re-render the image to the OGL control
	OGLRefreshGraph (mainPanel, OGLControlID);
}


/*---------------------------------------------------------------------------*/
/* Find all available com ports	and display on the ring menu                 */
/*---------------------------------------------------------------------------*/
void FindComPorts()
{
	int maxNum = 255;
	int portlist[maxNum];
	int nPorts, i;
	char deviceName[16];

	// Enumerate all available ports
	nPorts = LocateCom("", portlist, maxNum);

	// Sort the port numbers in ascending order
	qsort (portlist, nPorts, sizeof(int), IntCompareWrapper);
	
	// Display the found ports on the ring control
	if (nPorts)
	{
		for (i = 0; i < nPorts; i++)
		{
			sprintf(deviceName, "COM%d", portlist[i]);
			InsertListItem(mainPanel, PANEL_RING, -1, deviceName, portlist[i]);
		}
	}
}

// Wrapper function with the correct signature for the qsort
int IntCompareWrapper(const void *item1, const void *item2)
{
	// Call the original IntCompare function
	return IntCompare((void *)item1, (void *)item2);
}

void LoadConfiguration(void)
{
    // Create an IniText object
    iniHandle = Ini_New(0);

    // Read from the INI file
    Ini_ReadFromFile(iniHandle, INI_FILE_NAME);

    // Read COM port parameters from the INI file
    Ini_GetInt(iniHandle, "Settings", "PortNumber", &portNumber);
    Ini_GetInt(iniHandle, "Settings", "BaudRate", &baudRate);
    Ini_GetInt(iniHandle, "Settings", "Parity", &parity);
    Ini_GetInt(iniHandle, "Settings", "DataBits", &dataBits);
    Ini_GetInt(iniHandle, "Settings", "StopBits", &stopBits);
}

void SaveConfiguration(void)
{
    // Create an IniText object
    iniHandle = Ini_New(0);

    // Write COM port parameters to the INI file  
    Ini_PutInt(iniHandle, "Settings", "PortNumber", portNumber);
    Ini_PutInt(iniHandle, "Settings", "BaudRate", baudRate);
    Ini_PutInt(iniHandle, "Settings", "Parity", parity);
    Ini_PutInt(iniHandle, "Settings", "DataBits", dataBits);
    Ini_PutInt(iniHandle, "Settings", "StopBits", stopBits);

    // Write the INI file
    Ini_WriteToFile(iniHandle, INI_FILE_NAME);

    // Close the INI file
    Ini_Dispose(iniHandle);
}
 
int CVICALLBACK RingCallback (int panel, int control, int event,
								void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_RING_BEGIN_MENU: // Ring menu oppened
			ClearListCtrl (mainPanel, PANEL_RING); // Clear the ring menu
			FindComPorts();	 // Find com ports and display on the ring menu
			SetCtrlVal (mainPanel, PANEL_RING, currentPort); // Display the port number that was before openning the menu 
			break;

		case EVENT_COMMIT:
			GetCtrlVal (mainPanel, PANEL_RING, &currentPort); // Get the chosen port number
			if (portNumber != currentPort)					  // Check if a different port number was chosen
				SetCtrlAttribute (mainPanel, PANEL_CONNECT, ATTR_DIMMED, 0); // Enable connect button
			break;
	}
	return 0;
}

/*---------------------------------------------------------------------------*/
/* Show or hide the log 										             */
/*---------------------------------------------------------------------------*/
int CVICALLBACK ViewLogCallback (int panel, int control, int event,
								 void *callbackData, int eventData1, int eventData2)
{
	static int isOpen = 0; 
	switch (event)
	{
		case EVENT_LEFT_CLICK:
		case EVENT_LEFT_DOUBLE_CLICK:
			
			// Toggle the log display
			if (!isOpen)
				SetPanelSize (mainPanel, MAINPANEL_HEIGHT, MAINPANEL_WIDTH + textboxWidth + 15);
			else
				SetPanelSize (mainPanel, MAINPANEL_HEIGHT, MAINPANEL_WIDTH);
	
			isOpen = !isOpen;
			
			// Toggle the icon display
			SetCtrlAttribute (mainPanel, PANEL_VIEWLOG, ATTR_VISIBLE, !isOpen);
			SetCtrlAttribute (mainPanel, PANEL_HIDELOG, ATTR_VISIBLE, isOpen); 
			break;

		case EVENT_MOUSE_POINTER_MOVE:
			// Animate icons
			if (control != PANEL_CANVAS)
			{
				SetCtrlVal (mainPanel, PANEL_VIEWLOG, 1);
				SetCtrlVal (mainPanel, PANEL_HIDELOG, 1);
			}
			else
			{
				SetCtrlVal (mainPanel, PANEL_VIEWLOG, 0);
				SetCtrlVal (mainPanel, PANEL_HIDELOG, 0);
			}
			break;
	}
	return 0;
}

int CVICALLBACK TextBoxCallback (int panel, int control, int event,
								 void *callbackData, int eventData1, int eventData2)
{
	int rows, first;
	int	numVisibleLines;
	switch (event)
	{
		case EVENT_VSCROLL:
			GetCtrlAttribute(mainPanel, PANEL_TEXTBOX, ATTR_TOTAL_LINES, &rows);        // Get total lines
			GetCtrlAttribute(mainPanel, PANEL_TEXTBOX, ATTR_FIRST_VISIBLE_LINE, &first);  // Get the first visible line
			// Get number of visible lines in the text box  
			GetCtrlAttribute (mainPanel, PANEL_TEXTBOX, ATTR_VISIBLE_LINES, &numVisibleLines);
			
			if(rows - first == numVisibleLines)
			{
				scrolled = 0; // Scroll bar at the end
				SetCtrlAttribute (mainPanel, PANEL_SCROLLDOWN, ATTR_VISIBLE, 0);  // Hide the scroll down button
				SetCtrlAttribute(mainPanel, PANEL_TEXTBOX, ATTR_FIRST_VISIBLE_LINE, rows);  // Scroll to the end
			}
			else
			{
				scrolled = 1; // Scroll bar not at the end
				SetCtrlAttribute (mainPanel, PANEL_SCROLLDOWN, ATTR_VISIBLE, 1); // Display the scroll down button
			}
			break;
			
		case EVENT_MOUSE_POINTER_MOVE:
			SetCtrlVal (mainPanel, PANEL_SCROLLDOWN, 0); // Animate the scroll button
			break;
	}
  return 0;
}

/*---------------------------------------------------------------------------*/
/* Scroll to the end of the log text and enable autoscrolling	             */
/*---------------------------------------------------------------------------*/
int CVICALLBACK ScrollDownCallback (int panel, int control, int event,
									void *callbackData, int eventData1, int eventData2)
{
	int rows;
	switch (event)
	{
		case EVENT_LEFT_CLICK:
		case EVENT_LEFT_DOUBLE_CLICK:
			scrolled = 0; // Reset flag
			GetCtrlAttribute(mainPanel, PANEL_TEXTBOX, ATTR_TOTAL_LINES, &rows);        // Get total lines
			SetCtrlAttribute(mainPanel, PANEL_TEXTBOX, ATTR_FIRST_VISIBLE_LINE, rows);  // Scroll to the end
			SetCtrlAttribute (mainPanel, PANEL_SCROLLDOWN, ATTR_VISIBLE, 0);  // Hide the scroll down button
			break;
		case EVENT_MOUSE_POINTER_MOVE:
			SetCtrlVal (mainPanel, PANEL_SCROLLDOWN, 1); // Animate the scroll button
			break;
	}
	return 0;
}

int CVICALLBACK ViewPlaneCallback (int panel, int control, int event,
								   void *callbackData, int eventData1, int eventData2)
{
	int itemVal;
	switch (event)
	{
		case EVENT_VAL_CHANGED:
			GetCtrlVal (mainPanel, PANEL_VIEWPLANE, &itemVal);
			OGLSetCtrlAttribute (mainPanel, OGLControlID, OGLATTR_VIEW_DIRECTION, itemVal);
			if (itemVal == OGLVAL_USER_DEFINED)
			{
				OGLSetCtrlAttribute (mainPanel, OGLControlID, OGLATTR_VIEW_LONGITUDE, 45.0);
				OGLSetCtrlAttribute (mainPanel, OGLControlID, OGLATTR_VIEW_LATITUDE, 45.0); 
			}
			// Display plot
			OGLRefreshGraph(mainPanel, OGLControlID);
			break;
	}
	return 0;
}

void LogMessage(const char *format, ...)
{
    double currDateTime;
    char timeStamp[32];
    char logEntry[256];
    int rows;
				   
    // Get date and time and format into a string buffer according to TIME_FORMATSTRING
    GetCurrentDateTime(&currDateTime);
    FormatDateTimeString(currDateTime, TIME_FORMATSTRING, timeStamp, (int)sizeof(timeStamp));

    va_list args;
    va_start(args, format);

	// Write the formatted time to beggining of the message 
	int bytesWritten = sprintf(logEntry, "%s: ", timeStamp);
	
    // Use vsprintf to format the variable arguments
    vsprintf(logEntry + bytesWritten, format, args);

	va_end(args);
	
	// Insert message to the end
	InsertTextBoxLine(mainPanel, PANEL_TEXTBOX, -1, logEntry);

	if (!scrolled) // User is not interacting with the scroll bar
	{
		GetCtrlAttribute(mainPanel, PANEL_TEXTBOX, ATTR_TOTAL_LINES, &rows);   // Get total lines    
		SetCtrlAttribute(mainPanel, PANEL_TEXTBOX, ATTR_FIRST_VISIBLE_LINE, rows);  // Scroll to the end
	}
	
	// Write the log entry to the file
	fprintf(logFile, "%s\n", logEntry);
}

int CVICALLBACK ConfigCallback (int panel, int control, int event,
									   void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			if (control == PANEL_CONFIG)
			{
				// Config window oppened
				// Display the loaded or previously chosen parameters
				SetCtrlVal (configHandle, CONFIG_BAUDRATE, baudRate);
				SetCtrlVal (configHandle, CONFIG_PARITY, parity);
				SetCtrlVal (configHandle, CONFIG_DATABITS, dataBits);
				SetCtrlVal (configHandle, CONFIG_STOPBITS, stopBits);
				InstallPopup (configHandle);
			}
			else
			{
				// Config window closed
				// Retrieve the parameters upon closing
				GetCtrlVal (configHandle, CONFIG_BAUDRATE, &baudRate);
				GetCtrlVal (configHandle, CONFIG_PARITY, &parity);
				GetCtrlVal (configHandle, CONFIG_DATABITS, &dataBits);
				GetCtrlVal (configHandle, CONFIG_STOPBITS, &stopBits);
				RemovePopup (0);
			}
			break;
	}
	return 0;
}
