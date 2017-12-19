#ifndef Attr_H
#define Attr_H


struct Header{
    unsigned int version : 9;
    unsigned int type : 7;
    int length;
};


struct MessageAttribute{
    int type;
    int length;
    char payload[512];
};


struct Message{
    struct Header header;
    struct MessageAttribute attribute[2];
};

//Structure to link and hold client username and socket descriptor
struct InfoCli{
    int fd;
    char username[16];
    int NoofClients;
};

#endif
