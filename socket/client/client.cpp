#include <ext/stdio_filebuf.h>
#include <cstdio>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <iostream>

enum {
    DEFAULT_PORT = 12347,
    DEFAULT_READ_TIMEOUT = 5000,
    DEFAULT_WRITE_TIMEOUT = 5000
};
   
/**
 * Установка таймаута чтения или записи для сокета
 * Если в течении заданного времени в сокете не появится новых данных,
 * либо записанные данные не будут отправлены,
 * то операция завершится с ошибкой и сокет будет закрыт
 * Параметры:
 *   int sockfd - файловый дескриптор сокета
 *   long msec - время в миллисекундах
 *   int what = SO_RCVTIMEO - операция, по умолчанию - чтение
 *              SO_SNDTIMEO - запись
 * Возвращаемое значение:
 *   0 - успешно
 *   не 0 - ошибка
 */
int
sock_timeout(
    int sockfd,
    long msec,
    int what = SO_RCVTIMEO
) {
    struct timeval timeout;
    timeout.tv_sec = msec/1000;
    timeout.tv_usec = (msec%1000) * 1000;
    if(::setsockopt(sockfd, SOL_SOCKET, what, (char *)&timeout, sizeof(timeout)) < 0) {
        return -1;
    }
    return 0;
}

/**
 * Установка таймаута чтения из сокета
 * Если в течении заданного времени в сокете не появится новых данных,
 * то операция чтения завершится с ошибкой и сокет будет закрыт
 * Параметры:
 *   int sockfd - файловый дескриптор сокета
 *   long msec - время в миллисекундах
 * Возвращаемое значение:
 *   0 - успешно
 *   не 0 - ошибка
 */
int
sock_read_timeout(
    int sockfd,
    long msec
) {
    int status = sock_timeout(sockfd, msec, SO_RCVTIMEO);
    if(status < 0) {
        perror("sock_read_timeout");
    }
    return status;
}

/**
 * Установка таймаута записи в сокет.
 * Если в течении заданного времени данные не будут отправлены,
 * то операция записи завершится с ошибкой и сокет будет закрыт
 * Параметры:
 *   int sockfd - файловый дескриптор сокета
 *   long msec - время в миллисекундах
 * Возвращаемое значение:
 *   0 - успешно
 *   не 0 - ошибка
 */
int
sock_write_timeout(
    int sockfd,
    long msec
) {
    int status = sock_timeout(sockfd, msec, SO_SNDTIMEO);
    if(status < 0) {
        perror("sock_write_timeout");
    }
    return status;
}

#include <string.h>
#include <netdb.h> //gethostbyname

/**
 * Параметры
 *   argv[1] - имя хоста сервера в виде host:port
 *             port по умолчанию = 12347
 *             Имя сервера задавать обязательно.
 *   argv[2....n] - имена текстовых файлов, содержащих запросы
 *             Необходимо указать хотябы 1 файл
 */
int main(int argc, char *argv[]) {

    if(argc < 3) {
        std::cout << "Usage: mgt-client host[:port] file1 [file2 [...]]"
                  << std::endl;
        return EXIT_FAILURE;
    }

    in_port_t port = DEFAULT_PORT;
    {
        char *ddot;
        if((ddot = ::strchr(argv[1], ':'))) {
            *ddot = 0;
            if(ddot[1]) {
                if(int p = ::strtoul(&ddot[1], nullptr, 10)) {
                    port = p;
                }
            }
        }
    }
    std::string host(argv[1]);

    //Взять адрес хоста по имени
    hostent *record = gethostbyname(host.c_str());
    if(!record) {
        perror("Host lookup failure");
        return EXIT_FAILURE;
    }
    //Одному имени может соответствовать несколько адресов,
    //но пока возьмём первый
    in_addr *address = (in_addr *)record->h_addr_list[0];
    //Взять символьное представление адреса
    std::string ip(::inet_ntoa(*address));
    //std::cout <<"IP:" <<ip <<std::endl;

    //Подготовить структуру описания адреса
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = ::htons(port);
    //Преобразовать строковое представление адреса в двоичное
    if(::inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0) {
        perror("Invalid address");
        return EXIT_FAILURE;
    }
   
    //Создать сокет
    int client_socket;
    if ((client_socket = ::socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Create socket");
        return EXIT_FAILURE;
    }

    //Подключиться к серверу
    if (::connect(client_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return EXIT_FAILURE;
    }

    //Назначить таймауты для приёма и отправки данных
    sock_read_timeout(client_socket, DEFAULT_READ_TIMEOUT);
    sock_write_timeout(client_socket, DEFAULT_WRITE_TIMEOUT);

    //Создать поток для чтения данных из сокета
    __gnu_cxx::stdio_filebuf<char> inbuf(client_socket, std::ios::in);
    std::istream in(&inbuf);

    //Создать поток для записи данных в сокет
    __gnu_cxx::stdio_filebuf<char> outbuf(client_socket, std::ios::out);
    std::ostream out(&outbuf);

    for(int i = 2; i < argc; i++) {
        //Вывести имя файла для истории
        //std::cout <<argv[i] <<":";
        //std::flush(std::cout);

        //Попробовать открыьт файл
        std::ifstream infile(argv[i]);
        if(!infile.is_open()) {
            //Если открыть не получилось, идём дальше по списку
            perror("open");
            continue;
        }
        //Есть файл, отправляем его весь по строкам
        std::string buf;
        while(std::getline(infile, buf)) {
            if(!(out <<buf <<std::endl)) {
                perror("Send error");
                return EXIT_FAILURE;
            }
        }
        //Считываем ответ - одну строку
        std::getline(in, buf);
        //Печатаем ответ
        std::cout <<buf <<std::endl;
    }

    //Закрыть клиентский сокет
    ::close(client_socket);

    return EXIT_SUCCESS;
}
