#include <jni.h>
#include <string>
#include <netdb.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <android/log.h>
#include <unistd.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-lib::", __VA_ARGS__))

bool isGlobalUnicast(const struct in6_addr* addr) {
    return (addr->s6_addr[0] & 0xE0) == 0x20;
}

bool isPublicIPv4(const struct in_addr* addr) {
    uint32_t ip = ntohl(addr->s_addr);
    return !(
            (ip >= 0x0A000000 && ip <= 0x0AFFFFFF) ||  // 10.0.0.0/8
            (ip >= 0xAC100000 && ip <= 0xAC1FFFFF) ||  // 172.16.0.0/12
            (ip >= 0xC0A80000 && ip <= 0xC0A8FFFF) ||  // 192.168.0.0/16
            (ip >= 0xA9FE0000 && ip <= 0xA9FEFFFF)     // 169.254.0.0/16 (link-local)
    );
}


std::string getExternalIPAddress() {
    const char* googleDnsIp = "8.8.8.8";
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        LOGI("Failed to create socket");
        return "";
    }

    struct sockaddr_in serv;
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr(googleDnsIp);
    serv.sin_port = htons(53);

    int err = connect(sock, (const struct sockaddr*)&serv, sizeof(serv));
    if (err == -1) {
        LOGI("Failed to connect");
        close(sock);
        return "";
    }

    struct sockaddr_in name;
    socklen_t namelen = sizeof(name);
    err = getsockname(sock, (struct sockaddr*)&name, &namelen);
    if (err == -1) {
        LOGI("Failed to get sock name");
        close(sock);
        return "";
    }

    char buffer[INET_ADDRSTRLEN];
    const char* p = inet_ntop(AF_INET, &name.sin_addr, buffer, INET_ADDRSTRLEN);
    if (p == nullptr) {
        LOGI("Failed to convert IP to string");
        close(sock);
        return "";
    }

    close(sock);
    return std::string(buffer);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_interview_1cloudonix_MainActivity_getDeviceIp(JNIEnv* env, jobject) {
    std::string externalIp = getExternalIPAddress();
    if (!externalIp.empty()) {
        LOGI("External IP address: %s", externalIp.c_str());
        return env->NewStringUTF(externalIp.c_str());
    }

    struct ifaddrs* ifAddrStruct = nullptr;
    struct ifaddrs* ifa = nullptr;
    std::string ipv6Address;
    std::string publicIpv4Address;
    std::string privateIpv4Address;

    if (getifaddrs(&ifAddrStruct) == -1) {
        LOGI("getifaddrs failed");
        return env->NewStringUTF("");
    }

    for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }

        if (ifa->ifa_addr->sa_family == AF_INET6) {
            struct sockaddr_in6* addr = (struct sockaddr_in6*)ifa->ifa_addr;
            if (isGlobalUnicast(&addr->sin6_addr)) {
                char addressBuffer[INET6_ADDRSTRLEN];
                inet_ntop(AF_INET6, &(addr->sin6_addr), addressBuffer, INET6_ADDRSTRLEN);
                ipv6Address = addressBuffer;
            }
        } else if (ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in* addr = (struct sockaddr_in*)ifa->ifa_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(addr->sin_addr), addressBuffer, INET_ADDRSTRLEN);
            if (isPublicIPv4(&addr->sin_addr)) {
                publicIpv4Address = addressBuffer;
            } else if (privateIpv4Address.empty()) {
                privateIpv4Address = addressBuffer;
            }
        }
    }

    if (ifAddrStruct != nullptr) {
        freeifaddrs(ifAddrStruct);
    }

    std::string selectedIp;
    if (!ipv6Address.empty()) {
        selectedIp = ipv6Address;
    } else if (!publicIpv4Address.empty()) {
        selectedIp = publicIpv4Address;
    } else {
        selectedIp = privateIpv4Address;
    }

    LOGI("Selected IP address: %s", selectedIp.c_str());
    return env->NewStringUTF(selectedIp.c_str());
}