#include <WinSock2.h>
#include <WS2tcpip.h>
#include "http.h"


#include <time.h>
char headers[] =
"%s /%s HTTP/1.1\r\n\
	Host: %s\r\n\
	Connection: keep-alive\r\n\
	Cache-Control: max-age=0\r\n\
	Accept: */*\\\r\n\
	User-Agent: Mozilla/5.0 (Windows NT 6.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/50.0.2661.75 Safari/537.36\r\n\
	Accept-Language: en-US,en;q=0.8\r\n";

const struct URL* parseURL(char* link)
{



	int succ_parsing = 0; // Whether the parsing has been
	 // Page field of the uri if found




	struct URL* IKK = (struct URL*)malloc(sizeof(struct URL));


	memset(IKK->ip, 0, 100);
	memset(IKK->page, 0, 200);

	IKK->port = 80;
	succ_parsing = 0;

	// Set the proper tmp_source char*


	// Parsing the tmp_source char*
	if (sscanf(link, "http://%99[^:]:%i/%199[^\n]", IKK->ip, &IKK->port, IKK->page) == 3) { succ_parsing = 1; }
	else if (sscanf(link, "http://%99[^/]/%199[^\n]", IKK->ip, IKK->page) == 2) { succ_parsing = 1; }
	else if (sscanf(link, "http://%99[^:]:%i[^\n]", IKK->ip, &IKK->port) == 2) { succ_parsing = 1; }
	else if (sscanf(link, "http://%99[^\n]", IKK->ip) == 1) { succ_parsing = 1; }

	// Properly attaching the ip+page to a host
	if (succ_parsing) {
		return IKK;
	}

	return NULL;


}

char* getHttp(char* method, char* link, int size, char** mHeaders)
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	char* resv = (char*)malloc(1000);
	const struct URL* temp = parseURL(link);
	if (temp == NULL)return NULL;

	char header_buff[500];

	int status;
	struct addrinfo hints;
	// will point to the results
	struct addrinfo *res;
	memset(&hints, 0, sizeof hints); // make sure the struct is empty
	hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets


	// get ready to connect
	status = getaddrinfo(temp->ip, "80", &hints, &res);
	if (res != NULL)
	{


		sprintf(header_buff, headers, method, temp->page, temp->ip);

		for (int i = 0; i < size; i++)
		{
			strcat(header_buff, mHeaders[i]);
			strcat(header_buff, "\r\n");


		}
		strcat(header_buff, "\r\n");
		SOCKET x = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		int i1 = connect(x, res->ai_addr, res->ai_addrlen);
		int i2 = send(x, header_buff, strlen(header_buff), 0);
		int i3 = recv(x, resv, 10000 - 1, 0);

		freeaddrinfo(res);

		closesocket(x);
	}
	else
	{


	}
	WSACleanup();
	return resv;

}

char* Request(char* method, char* link, int size, char** mHeaders)
{
	return getHttp(method, link, size, mHeaders);
}
void http(fcall* cs)
{
	int i = cs->parm_count_c;

	char * met = *cs->func_parmeters[cs->parm_count_c - i--].val_str_ptr;
	char * link = *cs->func_parmeters[cs->parm_count_c - i--].val_str_ptr;
	int  size = *cs->func_parmeters[cs->parm_count_c - i--].value_int;
	char ** headers = cs->func_parmeters[cs->parm_count_c - i--].val_str_ptr;

	char* buff = getHttp(met, link, size, headers);

	cs->_return.val_str_ptr = get_pptr_string(buff);

}


type_def * get_type()
{
	type_def*  a;
	a->type_name = "http";
	//a.functions.root= 
	///func* function = add_function(&a.functions,":http", &a,4,http,0,0);
	//function->function_type= f_type::constr;
	///char * v[]= {"a","b","c","d"};
	////type* vs[]= {T_STRING,T_STRING,T_INT,T_ARRAY};
	/////func* functionn = add_function(&a.functions,"Request", T_STRING,4,http,v,vs);
	//////function->function_type= f_type::class_function;


	return a;

}



