#ifndef __MGT_H__
#define __MGT_H__

#include <unordered_map>
#include <unordered_set>
#include <set>
#include <string>
#include <vector>
#include <istream>

/**
 * Граф, сформированный из входного файла
 */
using Graph = std::unordered_map<std::string, std::set<std::string>>;

/**
 * Тип данных для подсчёта квадратов сумм
 */
using value_t = unsigned long long;

/**
 * Представление узла графа с дополнительными данными
 *   Node(value_t v = 0) : value(v) {}: конструктор с заданием собственного веса
 *   value_t value{}; //собственный исходный вес узла
 *   int comp_id = -1; //Индекс в векторе компонент связности графа
 *   value_t sum{}; //Сумма весов узла после редуцирования графа
 *   bool is_cutp{}; // true = узел является точкой сочленения
 *   bool is_reduced{};
 *   std::vector<value_t> subtree_sums;
 */
struct Node {
    Node(value_t v = 0) : value(v) {}
    value_t value{};
    int comp_id = -1;
    value_t sum{};
    bool is_cutp{};
    bool is_reduced{};
    std::vector<value_t> subtree_sums;
};

/**
 * Тип данных для массива узлов
 */
using Values = std::unordered_map<std::string, Node>;

/**
 * Структула, описывающая компоненту связности графа
 *   std::set<std::string> names : перечень узлов
 *   std::set<std::string> cutpoints : перечень точек сочленения
 *   value_t value{} : суммарный вес компоненты связности
 */
struct Component {
    std::set<std::string> names;
    std::set<std::string> cutpoints;
    value_t value{};
};

/**
 * Тип данных для массива компонентов связности
 */
using Components = std::vector<Component>;


/**
 * Обнулить счётчики строк ибайтов
 */
void
ser_zero_counters(
    void
);

/**
 * Вычитать из входного потока символы в заданном порядке
 * Параметры:
 *   std::istream& in - входной поток
 *   const char *expected_symbols - ожидаемые символы
 *   std::ostream& out - выходной поток для вывода ошибок, если будут
 *   bool verbose = false - 1=выводить сообщения об ошибках в выходной поток
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
);

/**
 * Считать из входного потока символы в строку до появления одного из
 * символов-ограничителей
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
);

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
int ser_in(
    std::istream& in,
    Graph& g,
    Values& v,
    std::ostream& out
);

/**
 * Разбор входного потока данных (без лишних заголовков),
 * построение графа, рачёт по графу, вывод результатов
 * Параметры:
 *   std::istream& in - входной поток
 *   std::ostream& out - выходной поток для вывода результата или ошибок
 *   std::string &point - наименоване зоны из запроса
 * Возвращаемое значение:
 *   0 - успешно
 *   не 0 - ошибка
 */
int
process(
    std::istream &in,
    std::ostream &out,
    std::string &point
);

/**
 * Разбор входного потока данных, включая заголовки http,
 * построение графа, рачёт по графу, вывод результатов,
 * в т.ч. заголовкв http
 * Параметры:
 *   std::istream& in - входной поток
 *   std::ostream& out - выходной поток для вывода результата или ошибок
     bool keepalive = false - 1=не завершать сеанс после вывода результатов
 * Возвращаемое значение:
 *   0 - успешно, можно продолжать сеанс
 *   < 0 - ошибка
 *   > 0 - успешно, завершить сеанс
 */
int
process_http_GET(
    std::istream &in,
    std::ostream &out,
    bool keepalive = false
);

#endif
