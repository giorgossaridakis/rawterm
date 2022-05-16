// rawterm, raw tcp/ip terminal
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <termios.h>

// constants
#define MAXBUFFER 256
const char *ASCIICODES[] = { "NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL", "BS", "HT", "LF", "VT", "FF", "CR", "SO", "SI", "DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB", "CAN", "EM", "SUB", "ESC", "FS", "GS", "RS", "US", "DEL" };
#define DEL 127
#define ESC 27
#define NL '\n'
#define LASTCODE 31
enum { CONTROL=1, FCNTL, REUSE, BIND, LISTEN, ADDRESSLOOKUP, CONNECTION };
const char *ERRORS[] = { "creating control socket", "fcntl on control socket", "socket reuse error", "port bind error", "socket listen error", "address lookup error", "connection error" };
enum { OFF = 0, ON };

// functions
// unblock stdin
short setraw()
{
  struct termios options;

        // unblock stdin
        fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK); // unblock
        tcgetattr( STDIN_FILENO, &options); // read current options
        options.c_lflag &= ~ICANON; // disable canonical line oriented input
        options.c_lflag &= ~ECHOCTL; // do not echo control chars as ^char, delete as ~?
        tcsetattr( STDIN_FILENO, TCSANOW, &options ); // write back to stdin

 return 0;
}

// TCP/IP telnet socket 
int clientsocket(char* host, char* port)
{
   int rc, s, arg;
   struct addrinfo hints, *res;
   
   // client socket setup
   memset(&hints, 0, sizeof(hints));

   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = AI_PASSIVE;

   if ((rc = getaddrinfo(host, port, &hints, &res) ) < 0 ) {
    perror(ERRORS[ADDRESSLOOKUP]);
    return 1;
   }
   
   s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
   if ( s < 0 ) {
    perror(ERRORS[CONTROL]);
    return 1;
   }

   if ( connect(s, res->ai_addr, res->ai_addrlen) < 0 ) {
    perror(ERRORS[CONNECTION]);
    return 1;
   }
   
   // unblock socket
   arg= fcntl(s, F_GETFL, NULL);
   arg |= O_NONBLOCK;
   fcntl(s, F_SETFL, arg);
  
   freeaddrinfo(res);
   
 return s;
}

// is screen printable character
short isprintablecharacter(int t)
{
  if ( t > 31 && t < 127 )
   return ON;
  
 return OFF;
}

// show usage
void showusage()
{
   printf("Usage:\n rawterm [options] <server> <port> \n\nAn unobstructed telnet terminal.\n\nOptions:\n");
  printf(" -d\t\tASCII decimal codes OFF\n -c\t\tASCII key codes OFF\n -s\t\tdisplay non-screen chars ON\n -a\t\tinterpet ANSI ON\n -l\t\tlog file OFF\n -o<filename>\toutput file, default <rawterm.log>\n      --help\tdisplay this help\n\nDistributed under the GNU Public licence.\n");
  
 exit (-1);
}

int main(int argc, char *argv[])
{
  char buf[2];
  ssize_t nread=1;
  int fd, opt, asciipos;
  short dec, cod, dis, ans, log, isprch;
  FILE *f;
  char filename[MAXBUFFER], server[MAXBUFFER], port[MAXBUFFER];
  strcpy(filename,"rawterm.log");
  dec=cod=log=ON; ans=dis=OFF;
    
     // parse command line
     while ((opt = getopt(argc, argv, "dclsao:")) != -1) {
      switch (opt) {
       case 'o':
        strcpy(filename, optarg);
       break;
       case 'd':
        dec=OFF;
       break;
       case 'c':
        cod=OFF;
       break;
       case 's':
        dis=OFF;
       break;
       case 'a':
        ans=ON;
       break;
       case 'l':
        log=OFF;
       break;
       default:
        showusage();
       break;
      }
     }
     if ( optind + 2 != argc )
      showusage();
     strcpy(server, argv[optind]);
     strcpy(port, argv[optind+1]);

    if ( setraw() )
     return(1);
    if ( log == ON ) {
     f=fopen(filename, "ab");
     if ( !f ) {
      perror("file io errror");
      return 1;
     }
    }
    
      // attempt connection
      if ( (fd=clientsocket(server, port)) == -1 )
       return 1;
    
      // read and display
      while ( nread ) {
        
       if ( (nread=read(STDIN_FILENO, buf, 1)) > 0 ) // read keyboard
        send( fd, buf, 1, 0 );       
       if ( (nread=recv(fd, buf, 1, 0)) > 0 ) { // read tcp/ip socket
        asciipos=((int)buf[0] == DEL ) ? LASTCODE : (int) buf[0];
        // apply filters
        if ( (isprch=isprintablecharacter(buf[0])) == OFF ) {
         if ( cod == ON )
          printf("[%s]", ASCIICODES[asciipos]);
         if ( log == ON ) {
          if ( dec == ON && cod == ON )
           fprintf(f, "[%d.%s]", (int)buf[0], ASCIICODES[asciipos]);
          if ( dec == ON && cod == OFF )
           fprintf(f, "[%d]", (int)buf[0]);    
          if ( dec == OFF && cod == ON )
           fprintf(f, "[%s]", ASCIICODES[asciipos]);  
         }
        }
        // apply filters
        if ( buf[0] == NL )
         isprch=ON;
        if ( ans == ON && buf[0] == ESC )
         isprch=ON;
        if ( dis == ON || isprch == ON )
         fprintf(stderr, "%c", buf[0]);
        if ( log == ON ) {
         fprintf(f, "%c", buf[0]);
         fflush(f);
        }
       }
       memset(buf, 0, 2);
       
      }

      if ( log == ON )
       fclose(f);
  
 return(0);
}
