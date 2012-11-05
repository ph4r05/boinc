/*
 * This file implemented by Balazs Bucsay and part
 * of the GI John distributed password cracker.
 * Copyright (c) 2008, 2009 GPLv2
 *
 * http://gijohn.info/
 * http://rycon.hu/
 *
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#ifndef _MSC_VER
#include <unistd.h>
#include <sys/file.h>
#else
#include <io.h>
#pragma warning ( disable : 4996 )
#define S_IRUSR _S_IREAD
#define S_IWUSR _S_IWRITE
#endif

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>


#if defined (__MINGW32__) || defined (_MSC_VER)
#include <conio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#define sleep(time) Sleep(1000*(time))
#define bcopy(src, dst, n) memcpy(src, dst, n)
#define bzero(src, n)      memset(src, 0x00, n)
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#ifndef __DJGPP__
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if !defined (_MSC_VER)
#include <termios.h>
#include <unistd.h>
#endif
#include <stdlib.h>
#else
#include <bios.h>
#endif
#if !defined(O_NONBLOCK) && defined(O_NDELAY)
#define O_NONBLOCK			O_NDELAY
#endif

#ifdef __CYGWIN32__
#include <string.h>
#include <sys/socket.h>
#ifndef __CYGWIN__
extern int tcgetattr(int fd, struct termios *termios_p);
extern int tcsetattr(int fd, int actions, struct termios *termios_p);
#endif
#endif
#endif /* !defined __MINGW32__ */


#include "formats.h"
#include "options.h"
#include "logger.h"
#include "gijohn.h"
#include "loader.h"
#include "compiler.h"
#include "path.h"
#ifdef HAVE_MPI
#include "john-mpi.h"
#endif

#ifndef ECHOPRT
# define ECHOPRT 0002000
#endif

/* external.c inside plaintext */
extern c_int ext_word[PLAINTEXT_BUFFER_SIZE];

/* john's options are in this struct */
extern struct db_main database;

/* just for to know where we are, debugging */
extern char *john_loaded_counts(void);

void update_words(char *fword, char *lword, int length);

char* sessionname;
extern char* rec_name;

// current length of first word
int length;

// set of characters that are used to generate single position in word
int charset[255];

// current length of word
int minlength;
int maxlength;

// current (left) word
int rword[32];
// actual last word defined  
char lastWord[255];

// length of charset
int charsetl;
int first_time;

// number of words to try
unsigned long long int modulo;

/* gijohn.c */

// are we initializing john for the first time in this run?
int firstrun = 1;
int getnewsid = 1;
char username[64], password[64];
struct parsedxml xmlxml;
char *crackedhash = NULL;
int crackedhashnum;
int actlen = 0;
struct hostent *host_entry;

int localConfiguration=0;
int useSeparateHashes=0;

/* yeah, it's pretty difficult :) */
unsigned long long int power(int base, int n)
{
	unsigned long long int p;
	int  i;

	p = 1;
	for (i = 1; i <= n; ++i) p *= base;

	return p;
}

/* init external loop */
/**
 * charset_ex = which characters to use for cracking
 * charsetl_ex = length of charset specify
 * fword = first word to try
 * lword = last word to try
 * 
 * egy = one
 * ketto = two
 * 
 * external - external mode, specified by config file. Initializes external mode
 * by defined interval piece. External mode uses another library and calls defined 
 * functions (callbacks) to initialize, generate, etc.
 * 
 * ext_word = current word to try, for external mode.
 */
void init_external(char *charset_ex, int charsetl_ex, char *fword, char *lword)
{
	int i;
        char tword[32];
        FILE * fd;

	modulo = 0;
	minlength = length = strlen(fword);
        maxlength = strlen(lword);
	charsetl = charsetl_ex;

	if (options.flags & FLG_VERBOSE) printf("[+] Original keyspace: %s - %s\n", fword, lword);
        // copy charset
	for (i=0;i<charsetl;i++)
	{
		charset[i] = charset_ex[i];
	}
        
        // backup real last word in chain
        strcpy((char*)&lastWord, lword);

        // different boundaries
        if (minlength!=maxlength){
            for(i=0; i<length; i++) tword[i] = charset[charsetl-1];
            tword[i]=0x00;
        } else {
            strcpy(tword, lword);
        }
        
        // compute modulo
	update_words(fword, tword, length);

        // empty output file for hashes
        if ((fd = fopen(GIJOHN_HASHES, "w+"))!=NULL){
            fclose(fd);
        }
        
	first_time = 1;
}

/**
 * computes new modulo, ext_word, 
 * @param fword         first word in interval
 * @param lword         last word in interval
 * @param length        length of the first word (strlen(fword)_
 */
void update_words(char *fword, char *lword, int length){
    int i, j, tword[32];
    unsigned long long int egy = 0, ketto = 0;
    for (i=0;i<length;i++)
	{
                // copying first word to external word buffer for external mode.
                // -> starting point
		ext_word[i] = fword[i];
		for (j=0;j<charsetl;j++)
		{
                        // rword, tword contains POSITIONS of characters in charset.
                        // rword = [ 0 0 0 5 ] == a a a f in case charset = abcdef...
                    
                        // if current charset char matches FIRST word on i position
                        // -> update rword[i]=j
			if (charset[j] == ext_word[i]) rword[i] = j;
                        // if current charset char matches LAST word on i position
                        // -> update tword[i]=j
			if (charset[j] == lword[i]) tword[i] = j;
		}
		egy += rword[i] * power(charsetl, (length-i-1));
		ketto += tword[i] * power(charsetl, (length-i-1));
	}
        // after this cycle, rword resp. tword should be completely set. It contains
        // position representations of first resp. last word. Good for incrementing and 
        // key space search.
        //
        // egy resp. ketto = converted first word, resp. last word to numbers of 
        // base charsetl (length of charset). It is numerical position system. Instead of
        // decimal base we are using charsetl base. This number is converted to decimal 
        // number by this procedure. 
        //
        // Thus if we have charset of length 25, we have number for example:
        // 1*25^5 + 6*25^4 + 4*25^3 + 4*25^3 + 0*25^2 + 0*25^1 + 24*25^0
        //
        // modulo is number of words between first word and last word using charset
        // for generation next words
	modulo = ketto-egy;

	if (options.flags & FLG_VERBOSE){
            printf("[+]   Current keyspace: %s - %s\n", fword, lword);
            printf("[+]   Interval's size:  %llu\n", modulo);
        }
        
        if (firstrun)
	{                
		firstrun = 0;
	}
                
	ext_word[length] = 0;
}

/**
 * Called when modulo key space is exhausted
 * @return 0 if we are done, 1 otherwise (new modulo is recomputed)
 */
int spaceExhaustedRecomputeModulo(){
    int i;
    char fword[32];
    char tword[32];
    
    if (length==maxlength) return 0;
    length++;
    
    if (length==maxlength){
        // last given interval - use proposed last word
        strcpy(tword, lastWord);
    } else {
        // still not last - generate last word from this size
        for(i=0; i<length; i++) tword[i] = charset[charsetl-1];
        tword[i]=0x00;
    }
    
    // generate new first word, if this happened, we are sure on the beginning
    for(i=0; i<length; i++) fword[i] = charset[0];
    fword[i] = 0x00;
    
    // recompute modulo
    update_words(fword, tword, length);
    
    return 1;
}

/* generate new words in the interval */
void generate_external()
{
	int i;
        
        // decrement number of passwords to try. If it is zero => key space exhausted
        // new: if exhausted and there still is different word length to try - change
        // word length and do it
	if (!modulo--) { 
            if (!spaceExhaustedRecomputeModulo()){
                ext_word[0] = 0; 
                return; 
            }
        }

        // "increment" rword by one using charset. If overflows, propagate higher
        // -> simple key space move by 1
	i = length - 1;
	while(++rword[i] == charsetl)
	{
	    rword[i] = 0;
	    i--;
	}

        // generate current external word from positional representation of left current word
	while (i < length)
	{
	    ext_word[i] = charset[rword[i]];
	    i++;
	}
	ext_word[length] = 0;
}

/* modified urlencode function */
void urlencode(char *s, char *t)
/* This function was written by Jon Forsberg (zzed) */
{
	char *p, *tp;

	if(t == NULL)
	{
		fprintf(stderr, "[-] Serious memory error...\n");
		exit(1);
	}

	tp = t;
	for(p = s; *p; p++)
	{
		if (((unsigned char)*p > 0x00 && (unsigned char)*p < ',') ||
		    ((unsigned char)*p > '9'  && (unsigned char)*p < 'A') ||
		    ((unsigned char)*p > 'Z'  && (unsigned char)*p < '_') ||
		    ((unsigned char)*p > '_'  && (unsigned char)*p < 'a') || 
		    ((unsigned char)*p > 'z'  && (unsigned char)*p < 0xA1))
		{
			sprintf(tp, "%%%02X", *p);
			tp += 3;
		}
		else
		{
			*tp = *p;
			tp++;
		}
	}
	*tp = '\0';

	return;
}

/* encode string to partially valid xml */
void xmlencode(char *s, char *t)
{
	char *p, *tp;

	if(t == NULL) 
	{
		fprintf(stderr, "[-] Serious memory error...\n");
		exit(1);
	}

	tp = t;

	for(p = s; *p != '\1'; p++)
	{
		if (*p == '<')
		{
			sprintf(tp, "&amp;lt;");
			tp += 8;
		}
		else if (*p == '>')
		{
			sprintf(tp, "&amp;gt;");
			tp += 8;
		}
		else if (*p == '&')
		{
			sprintf(tp, "&amp;");
			tp += 5;
		}
		else
		{
			*tp = *p;
			tp++;
		}
	}
	*tp = '\0';

	return;
}

/* split the server:port string */
void splitserver(char *server, int *port)
{
	char *temp;
	
	if (strstr(server, ":") == NULL) 
	{
		*port = 80;
	}
	else
	{
		temp = strstr(server, ":") + 1;
		*port = atoi(temp);
		*(temp - 1) = 0;
	}

	return;
}

/* get account from the gijohn.conf */
/**
 * Primitive username, password extraction from ini file.
 * 
 * @param user
 * @param password
 */
void getini(char *user, char *password)
{
#if defined (__MINGW32__) || defined (_MSC_VER)
        return;
#else
	FILE *hd;
	char buf[256];
	char *content;
	struct stat tempforstat;
	int actsize = 1024;
	int length, res = 0, i = 0, g = 0;
	struct termios org_opts, new_opts;

	if (stat("gijohn.conf", &tempforstat))
	{
		printf("[-] Couldn't find the gijohn.conf\n");
		exit(1);
	}
	memset(buf, '\0', 256);
	if ((content = malloc(sizeof(char)*1024)) == NULL)
	{
		fprintf(stderr, "Malloc error...%s %d\n", __FILE__, __LINE__);
		exit(1);
	}
	memset(content, '\0', actsize);

	hd = fopen("gijohn.conf", "r");
	while(fgets(buf, 256, hd))
	{
		if (strlen(content)+strlen(buf)>actsize)
		{
			actsize += 256;
			if ((content = realloc(content, sizeof(char) * actsize)) == NULL)
			{
				fprintf(stderr, "Malloc error...%s %d\n", __FILE__, __LINE__);
				exit(1);
			}
			memset(content+strlen(content), '\0', actsize - strlen(content));
		}
		memcpy(content+strlen(content), buf, strlen(buf));
	}
	fclose(hd);

/* lets split it */

	memset(user, '\0', 64);
	memset(password, '\0', 64);

	if (strstr(content, "username=") && strstr(strstr(content, "username="), "\r\n"))
	{
		if (content-strstr(strstr(content, "username="), "\r\n")>63) 
		{
			length = 63;
		}
		else 
		{
			length = strstr(strstr(content, "username="), "\r\n")-strstr(content, "username=")-strlen("username=");
		}
	}
	else if (strstr(content, "username=") && strstr(strstr(content, "username="), "\n"))
	{
		if (content-strstr(strstr(content, "username="), "\n")>63) 
		{
			length = 63;
		}
    		else 
		{
			length = strstr(strstr(content, "username="), "\n")-strstr(content, "username=")-strlen("username=");
		}
	}
	else
	{
		printf("[-] Please make a rigth gijohn.conf content (user)\n");
		exit(1);
	}
	strncpy(user, strstr(content, "username=")+strlen("username="), length);

	if (strstr(content, "password=") && strstr(strstr(content, "password="), "\r\n") && ((int)(strstr(strstr(content, "password="), "\r\n")-strstr(content, "password=")-9)!=0))
	{
		if (content-strstr(strstr(content, "password="), "\r\n")>63) 
		{
			length = 63;
		}
		else
		{
			length = strstr(strstr(content, "password="), "\r\n")-strstr(content, "password=")-strlen("password=");
		}
	}
	else if (strstr(content, "password=") && strstr(strstr(content, "password="), "\n") && ((int)(strstr(strstr(content, "password="), "\n")-strstr(content, "password=")-9)!=0))
	{
		if (content-strstr(strstr(content, "password="), "\n")>63) 
		{
			length = 63;
		}
		else 
		{
			length = strstr(strstr(content, "password="), "\n")-strstr(content, "password=")-strlen("password=");
		}
	}
	else
	{
		res = tcgetattr(STDIN_FILENO, &org_opts);
		memcpy(&new_opts, &org_opts, sizeof(new_opts));
		new_opts.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ECHOPRT | ECHOKE | ICRNL);
		tcsetattr(STDIN_FILENO, TCSANOW, &new_opts);
		
		printf("Password: ");
		g = getchar();
		while ((i < 63) && (g != '\n') && (g != '\r'))
		{
			password[i] = g;
			i++;
			g = getchar();
			
		}
		res = tcsetattr(STDIN_FILENO, TCSANOW, &org_opts);
		password[i] = 0;				
		printf("\n");
	}
	
	if (*password == 0) strncpy(password, strstr(content, "password=")+strlen("password="), length);

	return;
#endif
}

void parsexml2(char *xml, int ignoreNewHash)
{
	char temp[128];

	memset(temp, '\0', 128);

	if (strstr(xml, "<format>"))
	{
		memset(xmlxml.format, '\0', 64);
		strncpy(xmlxml.format, strstr(xml, "<format>")+strlen("<format>"), (strstr(xml, "</format>")-strstr(xml, "<format>")-strlen("<format>")<64)?(strstr(xml, "</format>")-strstr(xml, "<format>")-strlen("<format>")):63);
	}
	if (strstr(xml, "<start>"))
	{
		memset(xmlxml.keymap.firstword, '\0', 64);
		strncpy(xmlxml.keymap.firstword, strstr(xml, "<start>")+strlen("<start>"), (strstr(xml, "</start>")-strstr(xml, "<start>")-strlen("<start>")<64)?(strstr(xml, "</start>")-strstr(xml, "<start>")-strlen("<start>")):63);
	}
	if (strstr(xml, "<stop>"))
	{
		memset(xmlxml.keymap.lastword, '\0', 64);
		strncpy(xmlxml.keymap.lastword, strstr(xml, "<stop>")+strlen("<stop>"), (strstr(xml, "</stop>")-strstr(xml, "<stop>")-strlen("<stop>")<64)?(strstr(xml, "</stop>")-strstr(xml, "<stop>")-strlen("<stop>")):63);
	}
	if (strstr(xml, "<charset>"))
	{
		memset(xmlxml.keymap.charset, '\0', 256);
		strncpy(xmlxml.keymap.charset, strstr(xml, "<charset>")+strlen("<charset>"), (strstr(xml, "</charset>")-strstr(xml, "<charset>")-strlen("<charset>")<256)?(strstr(xml, "</charset>")-strstr(xml, "<charset>")-strlen("<charset>")):255);
	}
	if (strstr(xml, "<clearhashes>"))
	{
		xmlxml.clearhashes = atoi(strstr(xml, "<clearhashes>")+strlen("<clearhashes>"));
	}
	if (strstr(xml, "<upgrade>"))
	{
		xmlxml.upgrade = atoi(strstr(xml, "<upgrade>")+strlen("<upgrade>"));
	}
	if (strstr(xml, "<error>"))
	{
		memset(xmlxml.error, '\0', 1024);
		strncpy(xmlxml.error, strstr(xml, "<error>")+strlen("<error>"), (strstr(xml, "</error>")-strstr(xml, "<error>")-strlen("<error>"))<1024?strstr(xml, "</error>")-strstr(xml, "<error>")-strlen("<error>"):1023);
	}
	if (strstr(xml, "<sessionid>"))
	{
		memset(xmlxml.sessionid, '\0', 33);
		strncpy(xmlxml.sessionid, strstr(xml, "<sessionid>")+strlen("<sessionid>"), (strstr(xml, "</sessionid>")-strstr(xml, "<sessionid>")-strlen("<sessionid>"))<33?strstr(xml, "</sessionid>")-strstr(xml, "<sessionid>")-strlen("<sessionid>"):32);
	}
	if (strstr(xml, "<newhashes>") && !ignoreNewHash)
	{
		if ((xmlxml.newhashes = realloc(xmlxml.newhashes, sizeof(char) * (strstr(xml, "</newhashes>")-strstr(xml, "<newhashes>\n")-strlen("<newhashes>\n"))+1)) == NULL)
		{
			fprintf(stderr, "Malloc error...%s %d %ld\n", __FILE__, __LINE__, (strstr(xml, "</newhashes>")-strstr(xml, "<newhashes>\n")-strlen("<newhashes>\n"))+1);
			exit(1);
		}
		memset(xmlxml.newhashes, '\0', (strstr(xml, "</newhashes>")-strstr(xml, "<newhashes>\n")-strlen("<newhashes>\n"))+1);
		strncpy(xmlxml.newhashes, strstr(xml, "<newhashes>\n")+strlen("<newhashes>\n"), (strstr(xml, "</newhashes>")-strstr(xml, "<newhashes>\n")-strlen("<newhashes>\n")));
	}
	else 
	{
		xmlxml.newhashes[0] = 0;
	}
	if (strstr(xml, "<delhashes>"))
	{
		if ((xmlxml.delhashes = realloc(xmlxml.delhashes, sizeof(char) * (strstr(xml, " </delhashes>")-strstr(xml, "<delhashes>\n")-strlen("<delhashes>\n"))+1)) == NULL)
		{
			fprintf(stderr, "Malloc error...%s %d\n", __FILE__, __LINE__);
			exit(1);
		}
		memset(xmlxml.delhashes, '\0', (strstr(xml, " </delhashes>")-strstr(xml, "<delhashes>\n")-strlen("<delhashes>\n"))+1);
		strncpy(xmlxml.delhashes, strstr(xml, "<delhashes>\n")+strlen("<delhashes>\n"), (strstr(xml, " </delhashes>")-strstr(xml, "<delhashes>\n")-strlen("<delhashes>\n")));
	}
	else
	{
		xmlxml.delhashes[0] = 0;
	}
}

/* parse the response */
/**
 * Parse response XML - very simple XML parsing, no special XML libraries used.
 * Extracts directly hard coded parameters. Fills configuration structure struct parsedxml.
 * 
 * Recognized XML tags:
 * # format 
 * # start      first word of key space
 * # stop       last word of key space
 * # charset    charset to use to generate keys
 * # clearhashes
 * # upgrade
 * # error
 * # sessionid
 * # newhashes
 * # delhashes
 * 
 * #@param xml
 */
void parsexml(char *xml){
    parsexml2(xml, 0);
}

/* sometimes hostname resolve doesnt work... */
struct hostent *getthehostname(char *host)
{
	struct hostent *host_entry;

	if ((host_entry = gethostbyname(host)) == NULL)
	{
                fprintf(stderr, "[-] hostname lookup error...");
		exit(1);
	}
	return host_entry;
}

/* get a new connection */
int getconnection(struct hostent *host_entry, int port)
{
	int sd;
	struct sockaddr_in host_addr;

	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		return -1;
	}
        
	bzero(&host_addr, sizeof(host_addr));
	bcopy(host_entry->h_addr_list[0], &host_addr.sin_addr.s_addr, host_entry->h_length);
	host_addr.sin_family = host_entry->h_addrtype;
	host_addr.sin_port = htons(port);

	if (connect(sd, (struct sockaddr *)&host_addr, sizeof(struct sockaddr_in)) < 0)
	{
		return -1;
	}

	return sd;
}

/**
 * Download (HTTP GET) some XML from server
 * 
 * @param sd
 * @param xml
 * @param what
 * @param where
 * @param port
 * @param verbose
 * @param stripHdr      strip HTTP header
 */
void getxml2(int sd, char **xml, char *what, char *where, int port, int verbose, int stripHdr)
{
	FILE *stream;
	char buf[DOWNLOADSIZE];
	int actsize = DOWNLOADSIZE;
	int count = 0;
        int bodyNow=0;

	if ((*xml = malloc(sizeof(char)*actsize)) == NULL)
	{
		fprintf(stderr, "Malloc error...%s %d\n", __FILE__, __LINE__);
		exit(1);
	}
	memset(*xml, '\0', actsize);
	memset(buf, '\0', actsize);

	if ((stream = fdopen(sd, "r+")) == NULL)
	{
		fprintf(stderr, "fdopen error...\n");
		exit(1);
	}
	setbuf(stream, NULL);
	/* lighttpd doesnt likes the rfc? :'( */
	fprintf(stream, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", what, where);
	while(fgets(buf, DOWNLOADSIZE, stream))
	{
		count += strlen(buf);
		if (verbose) 
		{
			printf("[+] Downloading hashes: %d bytes\r", count);
		}
                
                if (stripHdr && !bodyNow && (strcmp("\r\n", buf)==0 || strcmp("\n", buf)==0)){
                    bodyNow=1;
                    continue;
                }
                
                if (stripHdr && !bodyNow) continue;
		if (strlen(*xml)+strlen(buf)>=actsize)
		{
			actsize += DOWNLOADSIZE;
			if ((*xml = realloc(*xml, sizeof(char) * actsize)) == NULL)
			{
				fprintf(stderr, "Not enough memory...\n");
				exit(1);
			}
			memset(*xml+strlen(*xml), '\0', actsize - strlen(*xml));
		}
		memcpy(*xml+strlen(*xml), buf, strlen(buf));
	}

	fclose(stream);

	return;
}

/**
 * Download (HTTP GET) some XML from server
 * 
 * @param sd
 * @param xml
 * @param what
 * @param where
 * @param port
 * @param verbose
 */
void getxml(int sd, char **xml, char *what, char *where, int port, int verbose){
    getxml2(sd, xml, what, where, port, verbose, 0);
}

/**
 * SIMPLE HTTP GET Download to file
 * @param dest
 * @param fname
 * @param verbose
 */
void downloadfile(char **dest, char * fname, int verbose){
    
    int sd, port, lenhttp;
    char *hostname, *tmp;
    
    lenhttp = strlen("http://");
    if(strncmp("http://", fname, lenhttp)!=0) {
        fprintf(stderr, "[-] URL is not supported: %s", fname);
        return;
    }
    
    if ((tmp=strstr(fname+lenhttp, "/"))==NULL){
        fprintf(stderr, "[-] URL does not contain file %s", fname);
        return;
    }
    
    hostname = (char*) malloc(sizeof(char) * (tmp-fname));
    strncpy(hostname, fname+lenhttp, tmp-fname-lenhttp);
    
    splitserver(hostname, &port);
    
    do
    {
            *xmlxml.error = 0;
            host_entry = getthehostname(hostname);
            if ((sd = getconnection(host_entry, port)) < 0)
            {
                    strncpy(xmlxml.error, "connection error",
                    strlen("connection error"));
                    printf("[-] Connection error\n[+]"
                    " Sleeping for %dsec... and reconnection\n", SLEEP_TIME);
                    sleep(SLEEP_TIME);
            }
            else
            {
                    getxml2(sd, dest, tmp, hostname, port, 0, 1);
                    close(sd);
                    
                    if (*xmlxml.error)
                    {
                            printf("[-] Error (new sessionid): %s\n[+]"
                            " Sleeping for %dsec... and resending\n", xmlxml.error, SLEEP_TIME);
                            sleep(SLEEP_TIME);
                    }
            }

    }
    while (*xmlxml.error);
    free(hostname);
}

/**
 * Load XML file contents from ordinary local file, or download it from web
 * @param xml
 * @param fname
 * @param verbose
 */
void getxmlfile(char **xml, char * fname, int verbose) {
    FILE * file;
    char buf[DOWNLOADSIZE];
    int actsize = DOWNLOADSIZE;
    int count = 0;

    // is url?
    if(strncmp("http://", fname, strlen("http://"))==0){
        downloadfile(xml, fname, verbose);
        return;
    }
    
    if ((file = fopen(fname, "r")) == NULL) {
        return;
    }
    
    if ((*xml = malloc(sizeof (char) *actsize)) == NULL) {
        fprintf(stderr, "Malloc error...%s %d\n", __FILE__, __LINE__);
        exit(1);
    }
    memset(*xml, '\0', actsize);
    memset(buf, '\0', actsize);

    while (fgets(buf, DOWNLOADSIZE, file)) {
        count += strlen(buf);
        if (strlen(*xml) + strlen(buf) >= actsize) {
            actsize += DOWNLOADSIZE;
            if ((*xml = realloc(*xml, sizeof (char) * actsize)) == NULL) {
                fprintf(stderr, "Not enough memory...\n");
                exit(1);
            }
            memset(*xml + strlen(*xml), '\0', actsize - strlen(*xml));
        }
        memcpy(*xml + strlen(*xml), buf, strlen(buf));
    }

    fclose(file);

    return;
}

/**
 * HTTP POST some xml to server
 * 
 * @param sd
 * @param xml
 * @param what
 * @param where
 * @param port
 * @param post
 */
void postxml(int sd, char **xml, char *what, char *where, int port, char *post)
{
	FILE *stream;
	char buf[DOWNLOADSIZE];
	int actsize = DOWNLOADSIZE;

	if ((*xml = malloc(sizeof(char)*actsize)) == NULL)
	{
		fprintf(stderr, "Malloc error...%s %d\n", __FILE__, __LINE__);
		exit(1);
	}
	memset(*xml, '\0', actsize);

	if ((stream = fdopen(sd, "r+")) == NULL)
	{
		fprintf(stderr, "fdopen error...\n");
		exit(1);
	}
	setbuf(stream, NULL);

	/* lighttpd doesnt like the rfc? :'( */
	fprintf(stream, "POST %s HTTP/1.0\r\nHost: %s\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %d\r\n\r\nhashes=%s\n", what, where, (int)(strlen(post)+7), post);
	while(fgets(buf, DOWNLOADSIZE, stream))
	{
		if (strlen(*xml)+strlen(buf)>=actsize)
		{
			actsize += DOWNLOADSIZE;
			if ((*xml = realloc(*xml, sizeof(char) * actsize)) == NULL)
			{
				fprintf(stderr, "Not enough memory...\n");
				exit(1);
			}
			memset(*xml+strlen(*xml), '\0', actsize - strlen(*xml));
		}
		memcpy(*xml+strlen(*xml), buf, strlen(buf));
	}

	fclose(stream);

	return;
}


/* storing the new cracked hashes */
/**
 * storing the new cracked hashes to central cracked hash structure
 * @param key
 * @param hash
 */
void setcrackedhash(char *key, char *hash)
{
	if (crackedhash == NULL)
	{
		actlen = strlen(key)+strlen(hash)+3;
		if ((crackedhash = malloc(sizeof(char) * actlen)) == NULL)
		{
			fprintf(stderr, "Malloc error...%s %d\n", __FILE__, __LINE__);
			exit(1);
		}
		memset(crackedhash, '\0', actlen);
		sprintf(crackedhash, "%s\1%s\n", key, hash);
		crackedhashnum = 1;
	}
	else
	{
		if ((crackedhash = realloc(crackedhash, sizeof(char) * (actlen+strlen(key)+strlen(hash)+2))) == NULL)
		{
			fprintf(stderr, "Malloc error...%s %d\n", __FILE__, __LINE__);
			exit(1);
		}
		memset(crackedhash+actlen, '\0', strlen(key)+strlen(hash)+2);
		sprintf(crackedhash+actlen-1, "%s\1%s\n", key, hash);
		actlen += strlen(key)+strlen(hash)+2;
		crackedhashnum++;
	}
}

/* make a partially valid xml for the system */
/**
 * Makes cracked hash report XML valid
 * @param post
 */
void makeitvalidxml(char **post)
{
	int actsize = 512, actpoint = 0;
	char temp[256], *temppass;

	if ((*post = malloc(sizeof(char)*actsize)) == NULL)
	{
		fprintf(stderr, "Malloc error...%s %d\n", __FILE__, __LINE__);
		exit(1);
	}
	memset(*post, '\0', actsize);
	sprintf(*post, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<crack>\n<sessionid>%s</sessionid>\n<format>%s</format>\n", xmlxml.sessionid, xmlxml.format);

	if (crackedhash != NULL)
	{
		while(strstr(crackedhash+actpoint, "\n"))
		{
			memset(temp, '\0', 256);
			strncpy(temp, crackedhash+actpoint, strstr(crackedhash+actpoint, "\n")-crackedhash-actpoint);
			actpoint = strstr(crackedhash+actpoint, "\n")-crackedhash+1;
			if (strlen(*post)+strlen(temp)+23>=actsize)
			{
				actsize += 512;
				if ((*post = realloc(*post, sizeof(char) * actsize)) == NULL)
				{
					fprintf(stderr, "Malloc error...%s %d\n", __FILE__, __LINE__);
					exit(1);
				}
				memset(*post+actsize-512, '\0', 512);
			}
			strncpy(*post+strlen(*post), "<plain hash=\"", strlen("<plain hash=\""));
			strncpy(*post+strlen(*post), strstr(temp, "\1")+1, strlen(temp)-(strstr(temp, "\1")-temp)-1);
			strncpy(*post+strlen(*post), "\">", 2);
			if ((temppass = malloc(sizeof(char)*(strstr(temp, "\1")-temp)*4+1)) == NULL)
			{
				fprintf(stderr, "Malloc error...%s %d\n", __FILE__, __LINE__);
				exit(1);
			}
			memset(temppass, '\0', sizeof(char)*(strstr(temp, "\1")-temp)*4+1);
			xmlencode(temp, temppass);
			strncpy(*post+strlen(*post), temppass, strlen(temppass));
			free(temppass);
			strncpy(*post+strlen(*post), "</plain>\n", strlen("</plain>\n"));
		}
	}
	if (strlen(*post)+strlen("</crack>\n")+1>=actsize)
	{
		actsize += strlen("</crack>\n")+1;
		if ((*post = realloc(*post, sizeof(char) * actsize)) == NULL)
		{
			fprintf(stderr, "Malloc error...%s %d\n", __FILE__, __LINE__);
			exit(1);
		}
		memset(*post+actsize-strlen("</crack>\n")-1, '\0', strlen("</crack>\n")+1);
	}
	strncpy(*post+strlen(*post), "</crack>\n", strlen("</crack>\n"));
}

/* setting infos from the computer */
/**
 * SEND basic XML reporting performance and format
 * @param post
 * @param username
 * @param password
 */
void makeformatandperformancexml(char **post, char *username, char *password)
{
	int actsize = 512;
	char temp[256];
	struct fmt_main *format;
	if ((*post = malloc(sizeof(char)*actsize)) == NULL)
	{
		fprintf(stderr, "Malloc error...%s %d\n", __FILE__, __LINE__);
		exit(1);
	}
	memset(*post, '\0', actsize);
	if (*username && *password) sprintf(*post, "user=%s&pass=%s&<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<firstinfo>\n<formats>\n", username, password);
	else sprintf(*post, "user=&pass=&<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<firstinfo>\n<formats>\n");
	if ((format = fmt_list))
	{
		do
		{
			memset(temp, '\0', 256);
			sprintf(temp, "<format>%s</format>\n", format->params.label);
			if (strlen(*post)+strlen(temp)+1>actsize)
			{
				actsize += 512;
				if ((*post = realloc(*post, sizeof(char) * actsize)) == NULL)
				{
					fprintf(stderr, "Malloc error...%s %d\n", __FILE__, __LINE__);
					exit(1);
				}
				memset(*post+actsize-512, '\0', 512);
			}
			memcpy(*post+strlen(*post), temp, strlen(temp));
		}
		while ((format = format->next));
	}
	if (strlen(*post)+strlen("</formats>\n<info>\n")+1>actsize)
	{
    		actsize += 512;
    		if ((*post = realloc(*post, sizeof(char) * actsize)) == NULL)
    		{
			fprintf(stderr, "Malloc error...%s %d\n", __FILE__, __LINE__);
			exit(1);
		}
		memset(*post+actsize-512, '\0', 512);
	}
	memcpy(*post+strlen(*post), "</formats>\n<info>\n", strlen("</formats>\n<info>\n"));

	if (strlen(*post)+strlen("<version></version>\n")+strlen(JOHN_VERSION)+1>actsize)
	{
		actsize += 512;
		if ((*post = realloc(*post, sizeof(char) * actsize)) == NULL)
		{
			fprintf(stderr, "Malloc error...%s %d\n", __FILE__, __LINE__);
			exit(1);
		}
		memset(*post+actsize-512, '\0', 512);
	}
	memcpy(*post+strlen(*post), "<version>", strlen("<version>"));
	memcpy(*post+strlen(*post), JOHN_VERSION, strlen(JOHN_VERSION));
	memcpy(*post+strlen(*post), "</version>\n", strlen("</version>\n"));
	if (strlen(*post)+strlen("<giversion></giversion>\n")+strlen(GIJOHN_VERSION)+1>actsize)
	{
		actsize += 512;
		if ((*post = realloc(*post, sizeof(char) * actsize)) == NULL)
		{
			fprintf(stderr, "Malloc error...%s %d\n", __FILE__, __LINE__);
			exit(1);
		}
		memset(*post+actsize-512, '\0', 512);
	}
	memcpy(*post+strlen(*post), "<giversion>", strlen("<giversion>"));
	memcpy(*post+strlen(*post), GIJOHN_VERSION, strlen(GIJOHN_VERSION));
	memcpy(*post+strlen(*post), "</giversion>\n", strlen("</giversion>\n"));

	if (strlen(*post)+strlen("</info>\n</firstinfo>")+1>actsize)
	{
		actsize += 512;
		if ((*post = realloc(*post, sizeof(char) * actsize)) == NULL)
		{
			fprintf(stderr, "Malloc error...%s %d\n", __FILE__, __LINE__);
			exit(1);
		}
		memset(*post+actsize-512, '\0', 512);
	}
	memcpy(*post+strlen(*post), "</info>\n</firstinfo>", strlen("</info>\n</firstinfo>"));
}

/* sending the result */
void sendtheresults()
{
	char *post, *post2, *xml;
        extern int aborted_gijohn;
	int sd;

	makeitvalidxml(&post);
        if (!localConfiguration){
            if ((post2 = malloc(sizeof(char)*strlen(post)*3+1)) == NULL)
            {
                    fprintf(stderr, "Malloc error...%s %d\n", __FILE__, __LINE__);
                    exit(1);
            }
            urlencode(post, post2);
            free(post);
            post = post2;
        }

	do
	{
            if (!localConfiguration){
		*xmlxml.error = 0;
		sd = getconnection(host_entry, gijohnport);
		postxml(sd, &xml, "/sendhashes.php", gijohnserver, gijohnport, post/*, 0*/);
		close(sd);
		parsexml(xml);
		free(xml);
		if (xmlxml.error[0])
		{
			printf("[-] Error (post): %s\n[+] Sleeping for %dsec... and resending\n", xmlxml.error, SLEEP_TIME);
			sleep(SLEEP_TIME);
		}
            } else {
                //FILE * file;
                int fd;
/*      
#ifdef HAVE_MPIXXX
                char fname[255];
                sprintf(fname, "johnresult_%02d.xml", mpi_id);
                if ((file=fopen(fname, "w+"))!=NULL){    
#else 
                if ((file=fopen("johnresult.xml", "w+"))!=NULL){    
#endif                
*/              
                printf("Using direct file. Stdouting... %s", post);
                
                if ((fd = open(path_expand(outXml!=NULL ? outXml : GIJOHN_HASHES),
                        O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR)) < 0)
                        pexit("open: %s", path_expand(GIJOHN_HASHES));
                
                {    
                    
#if defined(LOCK_EX) && OS_FLOCK
                    if (flock(fd, LOCK_EX)){ close(fd); continue; }
#endif
                    // send result only if there is something to report  
                    if (crackedhashnum>0){
                        if (write_loop(fd, post, strlen(post)) < 0) pexit("write");
                    }
#if defined(LOCK_EX) && OS_FLOCK
                    if (flock(fd, LOCK_UN)) { close(fd); break; }
#endif
                    if (close(fd)) pexit("close");
                    printf("[+] Dumped to file johnresult.xml\n");
                }
            }
	}
	while (*xmlxml.error);

	free(post);
	if (options.flags & FLG_VERBOSE) printf("[+] %d cracked hash sent\n", crackedhashnum);
	free(crackedhash);
	crackedhashnum = 0;
	crackedhash = NULL;

        // end now, this implementations is made to run continuously, after submitting
        // new hashes it is expected that new one will be assigned...
        aborted_gijohn=1;
	return;
}

/* destroy the session on the server side */
void destroysession()
{
	int sd;
	char *xml, query[256];

	sprintf(query, "/destroysession.php?sessionid=%s", xmlxml.sessionid);
	do
	{
		*xmlxml.error = 0;
		sd = getconnection(host_entry, gijohnport);
		getxml(sd, &xml, query, gijohnserver, gijohnport, 0);
		close(sd);
		parsexml(xml);
		free(xml);
		if (*xmlxml.error)
		{
			printf("[-] Error: destroying session: %s\n[+] Sleeping for %dsec... and resending\n", xmlxml.error, SLEEP_TIME);
			sleep(SLEEP_TIME);
		}
	}
	while (*xmlxml.error);

	if (options.flags & FLG_VERBOSE) printf("[+] Session destroyed\n[+] Thanks for using GI John!\n");

	return;
}

/* getting the new datas*/
/**
 * Retrieves new hashes to crack from server in form of XML. Handles session state, 
 * performance and status data sending.
 * @return 
 */
int getthenewpiece()
{
	int sd;
	char *xml, query[256], *post, *post2;

	if (firstrun)
	{
		splitserver(gijohnserver, &gijohnport);
		if ((xmlxml.newhashes = malloc(sizeof(char))) == NULL)
		{
			fprintf(stderr, "Malloc error...%s %d\n", __FILE__, __LINE__);
			exit(1);
		}
		*xmlxml.newhashes = 0;
		if ((xmlxml.delhashes = malloc(sizeof(char))) == NULL)
		{
			fprintf(stderr, "Malloc error...%s %d\n", __FILE__, __LINE__);
			exit(1);
		}
		*xmlxml.newhashes = 0;
		memset(xmlxml.format, '\0', 64);
		memset(xmlxml.keymap.firstword, '\0', 64);
		memset(xmlxml.keymap.lastword, '\0', 64);
		memset(xmlxml.keymap.charset, '\0', 256);
		memset(xmlxml.error, '\0', 1024);
		memset(xmlxml.sessionid, '\0', 33);
	}

        // check strlen of input files
        if (inpFileIncremental!=NULL){
            localConfiguration=1;
            printf("[+] Using local XML configuration: [%s]\n", inpFileIncremental);
        }
        
        // some settings can be specified on command line, but then must be separate
        // hash file present!
        if (inpCharset!=NULL
                && inpFormat!=NULL
                && inpFword!=NULL
                && inpLword!=NULL
                && inpFileHashes!=NULL){
            localConfiguration=1;
            printf("[+] Using command line configuration. fword: [%s] lword: [%s] charset: [%s]\n", inpFword, inpLword, inpCharset);
        }
        
        if (inpFileHashes!=NULL){
            useSeparateHashes=1;
            printf("[+] Using separate hashes: [%s]\n", inpFileHashes);
        }
        
	if (!localConfiguration && getnewsid)
	{		
		getini(username, password);		
		makeformatandperformancexml(&post, username, password);
		memset(password, 0, 64);
		if ((post2 = malloc(sizeof(char)*strlen(post)*3+1)) == NULL)
		{
			fprintf(stderr, "Malloc error...%s %d\n", __FILE__, __LINE__);
			exit(1);
		}
		urlencode(post, post2);
		free(post);
		post = post2;
		setbuf(stdout, NULL);
		if (options.flags & FLG_VERBOSE) printf("[+] Getting new session\n");

#if defined (__MINGW32__) || defined (_MSC_VER)
                ;
#else
		if (gijohnsmp > 1) {
                        int i;
			for (i = 1; i < gijohnsmp; i++) {
				if (!fork()) {
					sessionname = malloc(sizeof(char)*30);
					memset(sessionname, 0, 30);
					sprintf(sessionname, "gijohnfork_%d", i);
					rec_name = sessionname;
					break;
				}
			}
		}
#endif

		sprintf(query, "/newsession.php");

		do
		{
			*xmlxml.error = 0;
			host_entry = getthehostname(gijohnserver);
			if ((sd = getconnection(host_entry, gijohnport)) < 0)
			{
				strncpy(xmlxml.error, "connection error",
			 	strlen("connection error"));
				printf("[-] Connection error\n[+]"
				" Sleeping for %dsec... and reconnection\n", SLEEP_TIME);
				sleep(SLEEP_TIME);
			}
			else
			{
				postxml(sd, &xml, query, gijohnserver, gijohnport, post);
				close(sd);
				parsexml(xml);
				free(xml);
				if (*xmlxml.error)
				{
					printf("[-] Error (new sessionid): %s\n[+]"
					" Sleeping for %dsec... and resending\n", xmlxml.error, SLEEP_TIME);
					sleep(SLEEP_TIME);
				}
			}

		}
		while (*xmlxml.error);
		free(post);
		getnewsid = 0;
		if (options.flags & FLG_VERBOSE) printf("[+] New session is: %s\n", xmlxml.sessionid);
	}
	if (xmlxml.upgrade)
	{
		printf("[!] You have to upgrade your gijohn, because it's too old to use! Exiting...\n");
		destroysession();
		exit(1);
	}
        
        if (!localConfiguration){
            sprintf(query, "/getpieces.php?sessionid=%s&user=%s", xmlxml.sessionid, username);
            do
            {
                    *xmlxml.error = 0;
                    if ((sd = getconnection(host_entry, gijohnport)) < 0)
                    {
                            strncpy(xmlxml.error, "connection error",
                                strlen("connection error"));
                            printf("[-] Connection error\n[+]"
                            " Sleeping for %dsec... and reconnection\n", SLEEP_TIME);
                            sleep(SLEEP_TIME);
                    }
                    else
                    {
                            getxml(sd, &xml, query, gijohnserver, gijohnport, options.flags & FLG_VERBOSE);
                            close(sd);
                            parsexml(xml);
                            free(xml);
                            if (*xmlxml.error)
                            {
                                    printf("[-] Error (new keyspace): %s\n[+] "
                                    "Sleeping for %dsec... and resending\n", xmlxml.error, SLEEP_TIME);
                                    sleep(SLEEP_TIME);
                            }
                    }
            }
            while (*xmlxml.error);
        } else {
            if (inpFileIncremental){
                // if we have XML file specified, use that
                getxmlfile(&xml, inpFileIncremental, 1);
                printf("[+] XML loaded\n");
                parsexml2(xml, useSeparateHashes);
                printf("[+] Parsed\n");
                free(xml);
            } else {
                // here it seems we have everything in command line
		memset(xmlxml.format, '\0', 64);
		strcpy(xmlxml.format, inpFormat);
	
		memset(xmlxml.keymap.firstword, '\0', 64);
		strcpy(xmlxml.keymap.firstword, inpFword);
	
		memset(xmlxml.keymap.lastword, '\0', 64);
		strcpy(xmlxml.keymap.lastword, inpLword);
	
		memset(xmlxml.keymap.charset, '\0', 256);
		strcpy(xmlxml.keymap.charset, inpCharset);

		xmlxml.newhashes[0] = 0;
		xmlxml.delhashes[0] = 0;
            }
            
            // if we have separate hashes specified, use that, may be local file or URL
            if (useSeparateHashes){
                printf("[+] Using separate hashes: %s\n", inpFileHashes);
                getxmlfile(&xml, inpFileHashes, 1);
                printf("[+] XML loaded [%s]\n", xml);
                parsexml(xml);
                printf("[+] Parsed\n");
                free(xml);
            }
        }
        
	if (firstrun)
	{
		ldr_init_database(&database, &options.loader);
	}
        
	if (xmlxml.clearhashes)
	{
		memset(&database, '\0', sizeof(struct db_main));
		ldr_init_database(&database, &options.loader);
	}
        
	if (*xmlxml.error == 0)
	{
		if (*xmlxml.newhashes) ldr_load_xml_array(&database, xmlxml.newhashes, xmlxml.format);
		if (*xmlxml.delhashes) ldr_load_xml_delarray(&database, xmlxml.delhashes);
		if (*xmlxml.newhashes || *xmlxml.delhashes) ldr_fix_xmldatabase(&database, xmlxml.clearhashes | firstrun);
	}
	else
	{
		printf("[-] Error: %s\n", xmlxml.error); exit(1);
	}
        
	if ((options.flags & FLG_VERBOSE) || firstrun)
	{
		printf("[+] Loaded %s (%s [%s])\n",	john_loaded_counts(),
		database.format->params.format_name,
		database.format->params.algorithm_name);
	}
        
	if (firstrun)
	{
            if (!localConfiguration)
		printf("[+] Server: %s\n[+] Charset: %s\n[+] Charset length: %d\n", gijohnserver, xmlxml.keymap.charset, (int)strlen(xmlxml.keymap.charset));
            else
                printf("[+] Charset: %s\n[+] Charset length: %d\n", xmlxml.keymap.charset, (int)strlen(xmlxml.keymap.charset));
	}
        
	return 0;
}

