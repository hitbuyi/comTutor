#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
 
#define SOCKET_ERROR -1
#define PRINT(x) printf("%s\n",(x))
#define IP_SIZE     16 
int sockfd;
int ret;
char *send_data;
char *recv_data;
char From[128];
char To[128];
char Date[128];
char Subject[512];
char Context[6000]={0};
char buffer[1024];
#if 0           
struct hostent {
               char  *h_name;            /* official name of host */
               char **h_aliases;         /* alias list */
               int    h_addrtype;        /* host address type */
               int    h_length;          /* length of address */
               char **h_addr_list;       /* list of addresses */
           }

#endif
// 根据域名获取ip  
void printHost(struct hostent * host)
{
    printf("host.h_name:%s\n", host->h_name);
    printf("host.h_length:%d \n", host->h_length);
    printf("host.h_addr_list:%s \n", host->h_addr_list[0]);
    printf("host.h_addr_list:%s \n", host->h_addr_list[1]);
}
int get_ip_by_domain(const char *domain, char *ip)  
{  
    char **pptr;  
    struct hostent *hptr;  
  
    hptr = gethostbyname(domain);  
    if(NULL == hptr)  
    {  
        printf("gethostbyname error for host:%s \n", domain);  
        return -1;  
    }  
      printHost(hptr);
    for(pptr = hptr->h_addr_list ; *pptr != NULL; pptr++)  
    {  
        if (NULL != inet_ntop(hptr->h_addrtype, *pptr, ip, IP_SIZE) )  
        {  
            return 0; // 只获取第一个 ip  
        }  
    }  
  
    return -1;  
}

int connectHost(const char *smtpaddr)
{
    struct sockaddr_in serveAddr;
    bzero(&serveAddr,sizeof(serveAddr));
    serveAddr.sin_family = AF_INET;
    serveAddr.sin_port = htons(25);
    char serveIp[IP_SIZE];
    get_ip_by_domain("smtp.126.com" ,serveIp);
    printf("126_ip:%s\n",serveIp);
    if(inet_pton(AF_INET, serveIp, &serveAddr.sin_addr) <= 0 ) 
    {
        printf("inet_pton error!\n");
        return -1;
    };
    sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(sockfd < 0)
    {
        printf("Create socket error!\n");
        return -1;
    }
    if (connect(sockfd,(struct sockaddr *)&serveAddr,sizeof(serveAddr)) < 0)    
    {
        printf("Connect failed... \n");
        return -1;
    }
    printf("Connect to %s.... \n", serveIp);
     
    memset(buffer, 0, sizeof(buffer));
     
    if(recv(sockfd, buffer, sizeof(buffer), 0) < 0)
    {    
        printf("receive failed... \n");
        return -1;
    }
    else 
    {        
         printf("%s\n", buffer);
    }
    return sockfd;    
}
 

int getResponse()
{    
    memset(buffer,0,sizeof(buffer));
    ret = recv(sockfd,buffer,1024,0);
    if(ret == SOCKET_ERROR)
    {
        printf("receive nothing\n");
        return -1;
    }
    buffer[ret]='\0';
     
    if(*buffer == '5')
    {
        printf("the order is not support smtp host\n ");
        return -1;
    }

    printf("recieved buffer:%s\n",buffer);

    return 0;
}
 
int login(char *username,char *password)
{
    printf("login.....sockfd: %d\n",sockfd);
    char ch[100];
    if(username == "" || password == "")
        return -1;
     
    send_data = "HELO 163.com\r\n";
    printf("%s\n",send_data);
    ret = send(sockfd, send_data, strlen(send_data), 0);
    if(ret == SOCKET_ERROR)
    {
        close(sockfd);
        return -1;
    }         
    if(getResponse() < 0)
        return -1;
     
    send_data = "AUTH LOGIN\r\n";
    printf("%s\n",send_data);
    ret = send(sockfd, send_data, strlen(send_data), 0);
    if(ret == SOCKET_ERROR)
    {
        close(sockfd);
        return -1;
    }
    if(getResponse() < 0)
        return -1;
    sprintf(ch, "%s\r\n", username);
    printf("%s\n",ch);
    ret = send(sockfd, (char *)ch, strlen(ch),0);
    if(ret == SOCKET_ERROR)
    {
        close(sockfd);
        return -1;
    }
     
    if(getResponse() < 0)
         return -1;
     
    sprintf(ch,"%s\r\n",password);
    printf("%s\n",ch);
    ret = send(sockfd,(char *)ch,strlen(ch),0);
    if(ret == SOCKET_ERROR)
    {

        close(sockfd);
        return -1;
    }
     
    if(getResponse() < 0)
        return -1;
     
    return 0;
 
}
 
int sendmail(char* from, char * to, char * subject, char * context)
{
    if(from =="" ||to == ""||subject == ""||context == "")
    {
        printf("arguments error!\n");
        return -1;
    }
    
    sprintf(From, "MAIL FROM: <%s>\r\n", from);
    printf("%s\n",From);
     
    if((ret = send(sockfd, From, strlen(From), 0)) == SOCKET_ERROR)
    {
        close(sockfd);
        return -1;
    }
     
    if(getResponse() < 0)
        return -1;
     
    sprintf(To, "RCPT TO: <%s>\r\n", to);
    if((ret = send(sockfd, To, strlen(To),0)) == SOCKET_ERROR)
    {
        close(sockfd);
        return -1;
    }
    if(getResponse() < 0)
        return -1;
     
    send_data = "DATA\r\n";
    if((ret = send(sockfd,send_data,strlen(send_data),0)) == SOCKET_ERROR)
    {
        close(sockfd);
        return -1;
    }
    if(getResponse() < 0)
        return -1;
        
    memset(Subject, 0, sizeof(Subject));
    sprintf(Subject, "Subject: %s\r\n", subject);
    strcat(Context, Subject);
    strcat(Context, "\r\n\r\n");
    //strcat(Context,context);
    strcat(Context,"\r\n.\r\n");
    if((ret= send(sockfd, Context, strlen(Context), 0)) == SOCKET_ERROR)
    {
        close(sockfd);
        return -1;
    }
     
    printf("ret size:%d\n", ret);
    
    memset(buffer, '\0', sizeof(buffer));
    
    if(getResponse() < 0)
         return -1;    
 
    printf("QUIT!!!!!!!!!!!!!!!!\n");
    if((ret = send(sockfd,"QUIT\r\n",strlen("QUIT\r\n"), 0)) == SOCKET_ERROR)
    {
        close(sockfd);
        return -1;
    }
    if(getResponse() < 0)
        return -1;
 
    printf("Send Mail Successful!\n");
    return 0;
}
 

int main()
{    
    char from[128] = "funnylinux@163.com";
    char to[128] = "754056915@qq.com";

    char subject[512] = "test....";
    char context[6000] = "Just a test.........\n";
    char server[56]="smtp.163.com";

    char *name ="xxxxxxxxxxxxxx";//base64
    char *passwd = "xxxxxxxxxxxxxxx";//base64
 
    if(connectHost(server)<0)
    {
        printf("Can Not LOGIN !\n");
        return -1;
    }
 
    if(login(name, passwd) < 0)
    {
        fprintf(stderr,"Can Not LOGIN !\n");
        return -1;
    }
    sendmail(from, to, subject, context);
    return 0;
 }
