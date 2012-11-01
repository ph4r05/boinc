#define SLEEP_TIME 10
/* MAX PLAINTEXT LENGTH FOR BUFFER SIZE */
#define MAX_WORD  64
#define DOWNLOADSIZE 1024*1024
#define GIJOHN_HASHES			"johnresult_hash.xml"

char *inpFileIncremental;
char *inpFileHashes;

char *inpFormat;
char *inpCharset;
char *inpFword;
char *inpLword;

char *gijohnserver;
int gijohnport;
unsigned int gijohnsmp;

struct parsedxml
{
    char format[64];
    struct keyspace
    {
		 char firstword[64];
		 char lastword[64];
		 char charset[256];
    } 
    keymap;
    int clearhashes;
    int upgrade;  
    char *newhashes;
    char *delhashes;
    char error[1024];
    char sessionid[33];
};
