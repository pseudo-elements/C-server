#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>


char *contentType (char *filename) {
    char *extension = strrchr(filename, '.');
    if(extension == NULL) return "text/plain";
    if(strcmp(extension, ".html") == 0) return "text/html";
    if(strcmp(extension, ".css") == 0) return "text/css";
    if(strcmp(extension, ".js") == 0) return "application/javascript";
    if(strcmp(extension, ".jpg") == 0) return "image/jpeg";
    if(strcmp(extension, ".png") == 0) return "image/png";
    if(strcmp(extension, ".svg") == 0) return "image/svg";
    return "text/plain";
}

int main (void) {
    int server = socket(AF_INET, SOCK_STREAM, 0);
    if(server < 0){
        perror("server");
        exit(1);
    }

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    int resuse = 1;
    if(setsockopt(server,SOL_SOCKET, SO_REUSEADDR, &resuse, sizeof(resuse)) < 0){
        perror("setsockopt");
        exit(1);
    }

    if(bind(server, (struct sockaddr *)&address, sizeof(address)) < 0){
        perror("bind");
        exit(1);
    }
    
    if(listen(server, 10) < 0){
        perror("listen");
        exit(1);
    }

    printf("Listening on port :8080\n");

    while (1)
    {
        int client = accept(server,NULL,NULL);
        if (client < 0){
            perror("accept");
            exit(1);
        }

        char request[4096] = {0};
        ssize_t bytes_read = read(client, request, sizeof(request) - 1);

        if(bytes_read <= 0){
            close(client);
            continue;
        }

        char method[8], path[256], filename[512], header[256];
        if(sscanf(request, "%7s %255s", method, path) != 2) {
            close(client);
            continue;
        }

        printf("%s %s\n", method, path);

        if(snprintf(
            filename,
            sizeof(filename), 
            "www%s",
            strcmp(path, "/") == 0 ? "/index.html" : path
        ) < 0){
            perror("snprintf");
            close(client);
            continue;
        }

        struct stat info;
        int file = -1;

        //handle 404
        if(stat(filename, &info) < 0 || !S_ISREG(info.st_mode)){
            char *response = 
            "HTTP/1.0 404 not found\n";

            write(client, response, strlen(response));
            close(client);
            continue;
        }

        file = open(filename,O_RDONLY);
        if(file < 0){
            char *response = 
            "HTTP/1.0 404 not found\r\n"
            "Content-Type: text/html\r\n"
            "\r\n"
            "<p>Not Found<p>\n";

            write(client, response, strlen(response));
            close(client);
            continue;
        }

        printf("file descriptor: %d\n", file);

        //send content header
        int contentLength = snprintf(header, sizeof(header),
            "HTTP/1.0 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %lld\r\n"
            "\r\n",
            contentType(filename),
            (long long)info.st_size
        );

        write(client, header, contentLength);
        

        char buffer[4096];
        ssize_t count = read(file, buffer, sizeof(buffer));

        while (count > 0)
        {
            write(client, buffer, count);
            count = read(file, buffer, sizeof(buffer));
        }
        

        close(file);
        close(client);
    }
}
