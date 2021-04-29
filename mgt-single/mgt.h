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
 */
struct Node {
    Node(value_t v = 0) : value(v) {}
    value_t value{};
    int comp_id = -1;
    value_t sum{};
    bool is_cutp{};
    bool is_reduced{};
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
int ser_in(std::istream& in, Graph& g, Values& v, std::ostream& out);

#endif
