#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define BUF_SIZE 2048
#define SMALL_SIZE 100

void HTTPparser(int);
void HTTPresponse(char*,int);
char * content_type(char*);
int content_length(FILE*);
void send_error(FILE*);

int main(int argc, char*argv[]){
  int servfd, clientfd;
  int portno;
  socklen_t client;
  struct sockaddr_in serv_addr, cli_addr;

  if(argc!=2){
    perror("Error, Dont't input port n");
    exit(1);
  }

  //Get socket descriptor, PF_INET is IPv4 and SOCK_STREAM is TCP.
  servfd = socket(PF_INET,SOCK_STREAM,0);
  if(servfd<0){
    perror("ERROR on socket");
    exit(1);
  }

  //Initiate all bits of serv_addr to 0
  bzero((char*)&serv_addr, sizeof(serv_addr));
  portno = atoi(argv[1]);//converts from String to Integer
  serv_addr.sin_family = AF_INET;    //IPv4
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //INADDR_ANY is that server IP is always Local IP.
  serv_addr.sin_port = htons(portno); //convert from host data format to network data format

  //Bind the information of serv_addr to socket
  if(bind(servfd,(struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1){
    perror("Error on binding");
    exit(1);
  }

  //Ready to get calling of client
  if(listen(servfd,20) == -1){
    perror("Error on listening");
    exit(1);
  }

  //Server keep working until unless error is occured or server is ended
  while(1){
    client = sizeof(cli_addr);
    clientfd = accept(servfd,(struct sockaddr *)&cli_addr,&client); //Get descriptor of client
    if(clientfd < 0){
        perror("Error on accepting");
        continue;
      }
    HTTPparser(clientfd);


    close(clientfd);
  }

  close(clientfd);
  close(servfd);
  return 0;
}

/*
* Check that the request is HTTP request or not.
* parse filename from HTTP request
*/

void HTTPparser(int clientfd){

  char HTTP_request[BUF_SIZE];
  char method[10];
  char file_name[30];

  //Print the HTTP request
  read(clientfd,HTTP_request,1024);

  fputs(HTTP_request,stdout);

  if(strstr(HTTP_request,"HTTP/")==NULL){
    //Error 400 Bad request
    perror("400 Bad request");
    return;
  }

  strcpy(method, strtok(HTTP_request, " /"));
	strcpy(file_name, strtok(NULL, " /"));

  if(strcmp(method,"GET")!=0){
    //Not GET request
    perror("Not GET request");
    return;
  }
  HTTPresponse(file_name,clientfd);
  return;
}

/*
* If the request is right HTTP, send response
* Attach socket file stream to customed file structure pointer
* write responses into customed file structure pointer
*/

void HTTPresponse(char*file_name,int clientfd){
  FILE* file;

  // Connect the socket file to customed file pointer
  FILE* container;
  container = fdopen(dup(clientfd),"w");

  file = fopen(file_name, "r");
  fseek(file, 0, SEEK_END);
  int fileLength = ftell(file);
  fclose(file);

  char txtbuf[BUF_SIZE];
  int binbuf[BUF_SIZE];

  char protocol[] = "HTTP/1.1 200 OK\r\n";
  char server[] = "Server: Linux Web Server\r\n";
  char connection[] = "Connection: Keep-Alive\r\n";
  char keepAlive[] = "Keep-Alive : timeout = 10, max = 100\r\n";
  char acceptranges[] = "Accept-Ranges : bytes\r\n";
  char contentlength[SMALL_SIZE];
  char contenttype[SMALL_SIZE];
  sprintf(contentlength,"Content-Length:%d; charset=utf-8\r\n",fileLength);
  sprintf(contenttype,"Content-Type:%s; charset=utf-8\r\n\r\n",content_type(file_name));


  //Generate Date Format
  char date[100];
  time_t now = time(0);
  struct tm tm = *gmtime(&now);
  strftime(date, sizeof date, "Date: %a, %d %b %Y %H:%M:%S %Z\r\n", &tm);

  fputs(protocol,container);
  fputs(date,container);
  fputs(contentlength,container);
  fputs(server,container);
  fputs(keepAlive,container);
  fputs(acceptranges,container);
  fputs(connection,container);
  fputs(contenttype,container);

  if(!strcmp(content_type(file_name),"text/html")){
    // In case of html(text) file request
      file = fopen(file_name,"r");
      if(file == NULL){
        //file does not exist
        perror("Error to open file");
        //send_error(container);
        return;
      }else{
        while(fgets(txtbuf,BUF_SIZE,file)!=NULL){
          fputs(txtbuf,container);
          fflush(container);
        }
      }
  }else{
    // In case of binary file request
      file = fopen(file_name,"rb");
      if(file == NULL){
        //file does not exist
        perror("Error to open file");
        send_error(container);
        return;
      }else{
        while(fread(binbuf,sizeof(int),BUF_SIZE,file)!=0){
          fwrite(binbuf,sizeof(int),BUF_SIZE,container);
          fflush(container);

        }
      }
  }

  fflush(container);
  fclose(container);
  return;
}

/*
* Return the content type of requested file
*/
char * content_type(char*filename){
  char file_name[SMALL_SIZE];
  char whichtype[SMALL_SIZE];
  strcpy(file_name,filename);
  strtok(file_name,".");
  strcpy(whichtype,strtok(NULL,"."));

  if(!strcmp(whichtype,"html")||!strcmp(whichtype,"html")){
    return "text/html";
  }
  else if(!strcmp(whichtype,"gif")){
    return "image/gif";
  }else if(!strcmp(whichtype,"jpeg")){
    return "image/jpeg";
  }else if(!strcmp(whichtype,"mp3")){
    return "audio/mp3";
  }else if(!strcmp(whichtype,"pdf")){
    return "application/pdf";
  }else if(!strcmp(whichtype,"ico")){
    return "image/x-icon";
  }else{
    return "text/plain";
  }
}

/*
*  When file is not exist, Send 404 to client
*/

 void send_error(FILE*fp){
   char protocol[]="HTTP/1.1 404 Not Found\r\n";
   char cnt_type[]="Content-type:text/html\r\n\r\n";

   fputs(protocol,fp);
   fputs(cnt_type,fp);
 }
