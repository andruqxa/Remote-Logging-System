#pragma once


 // dim max buffer trimitere
#define CLIENT_BUFFER_SIZE 16384

// lungime max camp
#define MAX_FIELD_LENGTH 256

// lungime max json
#define MAX_JSON_MESSAGE_SIZE (CLIENT_BUFFER_SIZE - 1024)

// timeout comunicare 
#define CONNECTION_TIMEOUT_MS 5000
#define SEND_TIMEOUT_MS 3000
#define RECEIVE_TIMEOUT_MS 5000

// delay intre trimiteri consecutive
#define DELAY_BETWEEN_SENDS_MS 50

// configurare retry logic
#define MAX_RECONNECT_ATTEMPTS 3
#define RECONNECT_DELAY_MS 2000