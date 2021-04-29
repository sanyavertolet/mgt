#include <ext/stdio_filebuf.h>
#include <cstdio>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <netdb.h>
#include <arpa/inet.h>

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

#include <cctype>
#include <iomanip>
#include <string.h>

/**
 * Вывод в выходной поток строки с преобразованием url_encode, при котором
 * буквы, цифры и некоторые друие символы выводятся неизменно, а остальные
 * преобразуются в вид "%HH", где HH - это шестнадцатиричное число в верхнем
 * регистре. Неизменяемые символы, кроме букв и цифр, выбраны так же, как в браузере
 * chromium.
 * Параметры:
 *   std::ostream& out - выходной поток
 *   const std::string& str - исходная строка
 * Возвращаемое значение:
 *   true - успешно
 *   false - ошибка вывода
 */
bool url_encode(
    std::ostream& out,
    const std::string& str
) {
    //Символы, которые, как и буквы с цифрами, не меняются
    static const char unchanged[] = "`~!@$^&*()-_+{}[]\\|;:\",<.>/";
    for(char c: str) {
        if(::isalnum(unsigned(c)) || ::strchr(unchanged, c)) { //Буквы и цифры остаются
            if(!(out << c)) {
                return false;
            }
        } else { //Остальные символы кодируются как '%' и 2х-значное 16тиричное значение
            auto old_fill = out.fill('0');
            if(!(out <<'%' <<std::uppercase <<std::setw(2) <<std::hex
                <<unsigned(c) <<std::nouppercase <<std::dec <<std::setw(0))) {
                return false;
            }
            out.fill(old_fill);
        }
    }
    return true;
}

/**
 * Отправка запроса GET следующего вида:
 * GET /point=<url-кодированные данные для расчётов> HTTP/1.0\r\n
 * Host: host:port\r\n
 * User-Agent: mgt-client-http\r\n
 * Accept: text/html\r\n
 * \r\n     ----- пустую строку в конце
 * Параметры:
 *    std::istream& in - входной поток
 *    std::ostream& out - выходной поток
 *    const std::string& point - необязательное наименование ресурса сервера
 *    const std::string& host - имя (или адрес в текстовом виде) сервера
 *    unsigned port - порт сервера
 * Возвращаемое значение:
 *   true - успешно
 *   false - ошибка вывода
 */
bool http_GET_request(
    std::istream& in,
    std::ostream& out,
    const std::string& point,
    const std::string& host,
    unsigned port

) {
    if(!(out <<"GET /" <<point <<"=")) {
        return false;
    }
    std::string buf;
    while(std::getline(in, buf)) {
        buf += '\n'; //Добавить перевод строки, чтобы работал подсчёт строк
        //Отправить строку с перекодировкой
        if(!url_encode(out, buf)) {
            perror("Send error");
            return false;
        }
    }
    //Отправить хвост запроса и набор ещё каких-то полей запроса
    //Наверное, можно и без них
    return bool(out <<" HTTP/1.0\r\n"
                      "Host: " <<host <<":" <<port <<"\r\n"
                      "User-Agent: mgt-client-http\r\n"
                      "Accept: text/html\r\n"
                      "\r\n"); //Пустая строка в конце
}

/**
 * Параметры
 *   argv[1] - имя хоста сервера в виде host:port
 *             port по умолчанию = 12347
 *             Имя сервера задавать обязательно.
 *   argv[2....n] - имена текстовых файлов, содержащих запросы
 *             Необходимо указать хотябы 1 файл
 */
int main(int argc, char *argv[]) {
    if(argc < 2) {
        std::cout <<
        "Usage: mgt-client-http host[:port] file1 [file2 [...]]" <<std::endl;
        return EXIT_FAILURE;
    }

    in_port_t port = DEFAULT_PORT;
    {
        char *ddot;
        if((ddot = ::strchr(argv[1],':'))) {
            *ddot = 0;
            if(ddot[1]) {
                if(int p = ::strtoul(&ddot[1],nullptr,10)) {
                    port = p;
                }
            }
        }
    }
    std::string host(argv[1]);

    //Взять адрес хоста по имени
    hostent *record = gethostbyname(host.c_str());
    //Одному имени может соответствовать несколько адресов,
    //но пока возьмём первый
    in_addr *address = (in_addr *)record->h_addr_list[0];
    //Взять символьное представление адреса
    std::string ip = inet_ntoa(*address);
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
   
    for(int i = 2; i < argc; i++) {
        //Открыть файл запроса
        std::string point(argv[i]);
        std::ifstream infile(point);
        if(!infile.is_open()) {
            perror("open");
            continue;
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

        //Отправить запрос
        if(!http_GET_request(infile, out, point, host, port)) {
            perror("Error sending http request");
            return EXIT_FAILURE;
        }
        //Сбросить выходной поток, и, возможно, подождать отправки данных
        std::flush(out);

        //ЧТЕНИЕ ОТВЕТА

        std::string buf;
        int code;
        std::string status;
        //Первая строка имеет какой-то смысл
        //Например:
        //HTTP/1.1 200 OK - всё в порядке
        //         400 Bad Request - Ошибка текста запроса
        //         404 Not Found - Страница не найдена
        // ....

        //Пропустить HTTP/1.1 и считать код завершения
        in >>buf >>code;
        //Прочитать остатьо строки.
        //Там ещё только пробел в начале и \r в конце
        std::getline(in, buf);
        //Сформировать новую строку без лишних символов
        status = buf.substr(1, buf.size()-2);

        //Остальную часть ответа можно пропустить до пустой строки:
        while(std::getline(in, buf)) {
            if(buf.begin()[0] == '\r') {
                break;
            }
        }

        if(code == 200) { //OK
            //Дальше - данные
            while(std::getline(in, buf)) {
                std::cout <<buf <<std::endl;
            }
        } else {
            //Сообщить о неудачном завершении обмена
            std::cout <<"HTTP error code: " <<code <<" \"" <<status <<"\""<<std::endl;
        }
        //Закрыть клиентский сокет
        ::close(client_socket);
    }

    return EXIT_SUCCESS;
}
