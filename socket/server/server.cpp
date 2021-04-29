#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ext/stdio_filebuf.h>
#include <iostream>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <csignal>
#include <ctime>

#include "mgt.h"

enum {
    DEFAULT_PORT = 12347,
    DEFAULT_READ_TIMEOUT = 5000,
    DEFAULT_WRITE_TIMEOUT = 5000
};

static volatile std::sig_atomic_t GotSigTerm;
/**
 * Обработчик сигнала SIGTERM для хорошего завершения сервера
 * без прерывания сеанса связи
 */
void
sigterm_handler(
    int signal
) {
    GotSigTerm = signal;
}

static volatile std::sig_atomic_t GotSigPipe;
/**
 * Обработчик сигнала SIGPIPE, который приходит при
 * попытке вывода в неисправный сокет (клиент закрыл сокет)
 */
void
sigpipe_handler(
    int signal
) {
    GotSigPipe = signal;
}

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

/**
 * Параметры:
 *   argv[1] - порт прослушивания сервера
 *             Задавать обязательно
 */
int main(int argc, char *argv[]) {
    if(argc < 2) {
        std::cout <<
        "Usage: mgt-server port" <<std::endl;
        return EXIT_FAILURE;
    }

    //Порт сервера
    in_port_t port = std::stoi(argv[1], NULL, 10);

    //Создание сокета
    int server_fd;
    if((server_fd = ::socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    //Дополнительные параметры сокета:
    //SOL_SOCKET - передача параметров сокету
    //SO_REUSEADDR - Разрешает совместное использование порта
    //               несколькими процессами на разных интерфейсах
    //SO_REUSEPORT - Разрешает совместное использование порта
    //               несколькими процессами на одном и том же нтерфейсе
    int enable = 1; //1=установить, 0=сбросить
    if (::setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &enable, sizeof(enable))) {
        perror("setsockopt");
        return EXIT_FAILURE;
    }
    //Немедленно закрывать соединение после завершения сервера
    //не ждать вывода данных
    linger lin;
    lin.l_onoff = 0;
    lin.l_linger = 0;
    if(::setsockopt(server_fd, SOL_SOCKET, SO_LINGER, (const char *)&lin, sizeof(lin))) {
        perror("setsockopt(SO_LINGER)");
        return EXIT_FAILURE;
    }

    //Связать сокет с адресом
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;

    #undef htons
    address.sin_port = ::htons(port);

    int addrlen = sizeof(address);
    if(::bind(server_fd, (sockaddr*)(&address), sizeof(address)) < 0) {
        perror("bind failed");
        return EXIT_FAILURE;
    }

    {//Назначить обработчики сигналов
    struct sigaction sa{};
        sa.sa_handler = sigterm_handler;
        //sa.sa_flags = 0;
        ::sigaction(SIGTERM, &sa, NULL);
        sa.sa_handler = sigpipe_handler;
        //sa.sa_flags = 0;
        ::sigaction(SIGPIPE, &sa, NULL);
    }

    //Разрешить прослушивание сокета, 10 клиеннтов в очередь
    if (::listen(server_fd, 10) < 0) {
        perror("listen");
        return EXIT_FAILURE;
    }

    //Главный цикл сервера
    while(!GotSigTerm) {
        //Ждать подключения клиента
        int client_socket;
        //std::cout <<"Wait for client connection" <<std::endl;
        if((client_socket = ::accept(server_fd, (sockaddr *)(&address), (socklen_t*)(&addrlen))) < 0) {
            if(GotSigTerm) {
                //Если получен SIGTERM, то сразу выход
                //perror("got SIGTERM");
                break;
            }
            perror("accept");
            return EXIT_FAILURE;
        }

        //Клиент подключен
        //address.sin_addr - адрас клиента

        //std::cout <<"Accepted connection" <<std::endl;

        //Назачить таймауты на клиентском сокете
        sock_read_timeout(client_socket, DEFAULT_READ_TIMEOUT);
        sock_write_timeout(client_socket, DEFAULT_WRITE_TIMEOUT);

        //Создать буфер для чтения из клиентского сокета
        __gnu_cxx::stdio_filebuf<char> inbuf(client_socket, std::ios::in);
        //Создать поток ввода из клиентского сокета
        std::istream in(&inbuf);

        //Создать буфер для записи в клиентский сокет
        __gnu_cxx::stdio_filebuf<char> outbuf(client_socket, std::ios::out);
        //Создать поток вывода в клиентский сокет
        std::ostream out(&outbuf);

        //Выполнять цикл обработки запросов
        while(process(in, out) == 0) {
            ;
        }

        if(!GotSigPipe) {
            //Если клиентский сокет всё ещё живой,
            //то сбросить буфер (за одно подождать отправки данных)
            std::flush(out);
        } else {
            //Если сокет поломался, то сообщить
            perror("Send error");
            GotSigPipe = 0;
        }

        //Закрыть клиентский сокет
        ::close(client_socket);
    }

    return EXIT_SUCCESS;
}
