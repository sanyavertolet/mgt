#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <utility>
#include <unordered_map>
#include <map>
#include <list>

#include "mgt.h"

/**
 * Хранение последнего символа,
 * подсчёт количества обработанных строк и символов
 * Не кошерно, но пока так
 */
static int ser_last_char = 0; //Последний считанный не пробельный символ
static int line_num; //Количество обработанных строк на одном блоке данных
static int char_num; //Количество обработанных символов на одном блоке данных

/**
 * Обнулить счётчики строк ибайтов
 */
void
ser_zero_counters(
    void
) {
    line_num = 0;
    char_num = 0;
}

/**
 * Считать очередной символ из входного потока
 * там же посчитать строки и байты.
 * Подсчёт строк и символов начинается с первого непробельного символа
 * Параметры:
 *   std::istream& in - входной поток
 * Возвращаемое значение:
 *   -1 - ошибка чтения
 *   >=0 - считанный символ
 */
int
ser_get_char(
    std::istream& in
) {
    char ci;
    in >>std::noskipws;
    if(in >>ci) {
        //Подсчёт строк и символов начинается
        //с первого непробельного символа
        if(char_num || !std::isspace(ci)) {
            ++char_num;
            if(ci == '\n') {
                ++line_num;
            }
        }
        return ci;
    }
    return -1;
}

/**
 * Сформировать строку - заголовок сообщения об ошибке
 */
std::string
ser_err(
) {
    std::string s("Input format violation at line ");
    s += std::to_string(line_num+1);
    s += " char ";
    s += std::to_string(char_num);
    s += ": ";
    return s;
}

/**
 * Вычитать из входного потока символы в заданном порядке, игнорируя пробелы.
 * Параметры:
 *   std::istream& in - входной поток
 *   const char *expected_symbols - ожидаемые символы
 *   std::ostream& out - выходной поток для вывода ошибок, если будут
 *   bool verbose = false - true=выводить сообщения об ошибках в выходной поток
 * Возвращаемое значение:
 *   0 - символы считаны
 *   не 0 - ошибка
 */
int
ser_expect_char(
    std::istream& in,
    const char *expected_symbols,
    std::ostream& out,
    bool verbose = false
) {
    char expected_char;
    int next_char = 0;
    for(int i = 0; (expected_char = expected_symbols[i]) != 0; ++i) {
        while((next_char = ser_get_char(in)) > 0) {
            if(std::isspace(next_char)) {
                continue;
            }
            ser_last_char = next_char;
            if(next_char != expected_char) {
                if(verbose) {
                    out <<ser_err()
                        <<"expected char "
                        <<expected_char
                        <<" but found "
                        <<char(next_char)
                        <<std::endl;
                }
                return next_char;
            } else {
                break;
            }
        }
        if(next_char == 0) {
            out <<ser_err()
                <<"expected char "
                <<expected_char
                <<" but found 0-symbol"
                <<std::endl;
            return -1;
        }
        if(in.eof()) {
            out <<ser_err()
                <<"expected char "
                <<expected_char
                <<" but end of file reached"
                <<std::endl;
            return -1;
        }
    }
    return 0; //ok
}

/**
 * Считать из входного потока символы в строку до появления одного из
 * символов-ограничителей. Игнорируются символы "пробел" и "табуляция".
 * Параметры:
 *   std::istream& in - входной поток
 *   std::string& s, - строка для накопления данных
 *   const char *stop_symbols, - массив символов - ограничителеё
 *   std::ostream& out - выходной поток для вывода ошибок, если будут
 * Возвращаемое значение:
 *   0 - символы считаны
 *   не 0 - ошибка
 */
int
ser_read_until(
    std::istream& in,
    std::string& s,
    const char *stop_symbols,
    std::ostream& out
) {
    char next_char;
    static const char *spaces = "\t ";
    while((next_char = ser_get_char(in)) > 0) {
        if(s.size() == 0 && ::isspace(next_char)) {
            continue;
        }
        ser_last_char = next_char;
        if(::strchr(stop_symbols, next_char)) {
            return 0;
        }
        if(::strchr(spaces, next_char)) {
            continue;
        }
        s += next_char;
    }
    if(next_char == 0) {
        out <<ser_err()
            <<"unexpected 0-symbol"
            <<std::endl;
        return -1;
    }
    out <<ser_err()
        <<"unexpected end of file"
        <<std::endl;
    return -1;
}

/**
 * Выполнение выражения, анализ кода завершения и выход, если код не 0
 * Если код завершения ==0, то можно продолжать
 */
#define OK(expr) {int err = expr; if(err != 0) return err;}

/**
 * Считывание имени, залючённого в одинарные кавычки, например, 'A'
 * Параметры:
 *   std::istream& in - входной поток
 *   std::string& name - строка для сохранения имени
 *   std::ostream& out - выходной поток для вывода ошибок, если будут
 * Возвращаемое значение:
 *   0 - успешно
 *   не 0 - ошибка
 */
int
ser_get_name(
    std::istream& in,
    std::string& name,
    std::ostream& out
) {
    OK(ser_expect_char(in, "\'", out, true));
    OK(ser_read_until(in, name, "\' \t\n", out));
    return 0;
}

/**
 * Считывание массива ссылок вида ['A' = 'B'] и формирование графа.
 * Считывание продолжается пока после ссылки стоит запятая
 * Параметры:
 *   std::istream& in : входной поток
 *   Graph& graph : контейнер для формипрвания графа
 *   std::ostream& out : выходной поток для вывода ошибок, если будут
 * Возвращаемое значение:
 *   0 - успешно
 *   не 0 - ошибка
 */
int
ser_get_graph(
    std::istream& in,
    Graph& graph,
    std::ostream& out
) {
    do {
        std::string n1;
        std::string n2;
        OK(ser_expect_char(in, "[", out, true));
        OK(ser_get_name(in, n1, out));
        OK(ser_expect_char(in, ",", out, true));
        OK(ser_get_name(in, n2, out));
        OK(ser_expect_char(in, "]", out, true));
        //links.push_back({n1, n2});            for std::list<std::pair<std::string, std::string>>
        graph[n1].insert(n2);
        graph[n2].insert(n1);
    } while(ser_expect_char(in, ",", out, false) == 0);
    return 0;
}


/**
 * Считывание значения вида :dd
 * Параметры:
 *   std::istream& in - входной поток
 *   int& value - ссылка для возврата значения
 *   std::ostream& out - выходной поток для вывода ошибок, если будут
 * Возвращаемое значение:
 *   0 - успешно
 *   не 0 - ошибка
 */
int
ser_get_val(
    std::istream& in,
    int& value,
    std::ostream& out
) {
    OK(ser_expect_char(in, ":", out, true));
    std::string valstr;
    OK(ser_read_until(in, valstr, ",}] \t\n", out));
    size_t pos = 0;
    int v = std::stoi(valstr, &pos);
    if(pos != valstr.size()) {
        out <<ser_err()
            <<"value "
            <<valstr
            <<"not an integer"
            <<std::endl;
        return -1;
    }
    value = v;
    return 0;
}

/**
 * Считывание массива значений вида 'A':dd и формирование контейнера
 * считывание продолжается пока после значения стоит запятая
 * Параметры:
 *   std::istream& in - входной поток
 *   Value& values - контейнер для хранения значений
 *   std::ostream& out - выходной поток для вывода ошибок, если будут
 * Возвращаемое значение:
 *   0 - успешно
 *   не 0 - ошибка
 */
int
ser_get_values(
    std::istream& in,
    Values &values,
    std::ostream& out
) {
    do {
        std::string name;
        OK(ser_get_name(in, name, out));
        int value;
        OK(ser_get_val(in, value, out));
        values.insert({name, Node(value)});
        if(isspace(ser_last_char)) {
            ser_expect_char(in, ",", out, false);
        }
    } while(ser_last_char == ',');
    return 0;
}

/**
 * Разбор входного потока и формирование контейнеров графа и
 * массива узлов графа
 * Параметры:
 *   std::istream& in : входной поток
 *   Graph& g : сформированный граф
 *   Values& v : сформированный массив узлов графа
 *   std::ostream& out - выходной поток для вывода ошибок, если будут
 * Возвращаемое значение:
 *   0 - успешно
 *   не 0 - ошибка
 */
int
ser_in(
    std::istream& in,
    Graph &g,
    Values &v,
    std::ostream& out
) {
    ser_zero_counters();
    OK(ser_expect_char(in, "{[", out, true));
    OK(ser_get_graph(in, g, out));
    if(ser_last_char != ']') {
        std::string prefix = ser_err();
        out <<prefix
            <<"expected char ] after link list"
            <<std::endl;
        return -1;
    }
    OK(ser_expect_char(in, ",{", out, true));
    OK(ser_get_values(in, v, out));
    if(ser_last_char != '}') {
        std::string prefix = ser_err();
        out <<prefix
            <<"expected char } after value list"
            <<std::endl;
        return -1;
    }
    OK(ser_expect_char(in, "}", out, true));
    for(auto value : v) {
        g[value.first];
    }
    for(auto &[from, tos] : g) {
        v[from];
        for(auto to : tos) {
            v[to];
        }
    }

    return 0;
}
