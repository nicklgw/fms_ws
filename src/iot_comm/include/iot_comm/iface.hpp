
#ifndef IFACE__IFACE_HPP_
#define IFACE__IFACE_HPP_

#include <iomanip>
#include <iostream>
#include <cstdlib>
#include <iostream>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <uuid/uuid.h>

int get_mac_ip(const std::string &iface, std::string &mac, std::string &ip)
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) 
    {
        std::perror("socket");
        return -1;
    }

    // eth0 是你想要查询的网络接口名称
    // const char *iface = "eth0";
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, iface.c_str(), sizeof(ifr.ifr_name));

    // 获取MAC地址
    if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) 
    {
        std::perror("ioctl SIOCGIFHWADDR");
        close(fd);
        return -1;
    }

    char buffer[256] = { 0 };

    unsigned char *ether = (unsigned char *)ifr.ifr_hwaddr.sa_data;
    std::sprintf(buffer, "%02x%02x%02x%02x%02x%02x", ether[0], ether[1], ether[2], ether[3], ether[4], ether[5]);
    mac = buffer;

    // 获取IP地址
    struct sockaddr_in addr;
    ifr.ifr_addr.sa_family = AF_INET;
    if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) 
    {
        std::perror("ioctl SIOCGIFADDR");
        close(fd);
        return -1;
    }

    memcpy(&addr, &ifr.ifr_addr, sizeof(addr));
    ip = inet_ntoa(addr.sin_addr);
 
    close(fd);
    fd = -1;
    
    return 0;
}

#endif  // IFACE__IFACE_HPP_
