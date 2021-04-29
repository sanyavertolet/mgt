#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <list>
#include <string>
#include <fstream>
#include <iterator>

#include "mgt.h"

//Разбор входного потока и преобразование его во внутренние структуры данных.
void parse(Graph &graph, Values &values, std::istream &stream, std::ostream &out) {
    if(ser_in(stream, graph, values, out)) {
        return ;
    }
}

template<typename Container>
void print_cont(Container c) {
    for(auto &[name, vitality] : c) {
        std::cout << name << " = " << vitality;
        std::cout << std::endl;
    }
}

//Поиск точек сочленения, точек, лежащих в одной компоненте связности, суммы по одной компоненте связности.
value_t dfs (
        Graph &g,
        std::set<std::string> &used,
        Values& v,
        std::set<std::string> &cutpoints,
        std::string node,
        int comp_id,
        std::string parent = {}
        ) {
    v[node].comp_id = comp_id;
    static std::map<std::string, int> tin;
    static std::map<std::string, int> fup;
    static int timer;
    value_t value = 0;
    used.insert(node);
    tin[node] = fup[node] = timer++;
    int children = 0;
    for (auto to = g[node].begin(); to != g[node].end(); ++to) {
        if (*to == parent) {
            continue;
        }
        if (used.find(*to) != used.end()) {
            fup[node] = std::min(fup[node], tin[*to]);
        }
        else {
            value += dfs(g, used, v, cutpoints, *to, comp_id, node);
            fup[node] = std::min(fup[node], fup[*to]);
            if (fup[*to] >= tin[node] && !parent.empty()) {
                cutpoints.insert(node);
                v[node].is_cutp = true;
            }
            ++children;
        }
    }
    if (parent.empty() && children > 1) {
        cutpoints.insert(node);
        v[node].is_cutp = true;
    }
    return v[node].value + value;
}

//Разделение исходного графа на компоненты связности
void make_components(Graph &g, Values &v, Components &comps) {
    for(auto &[name, links] : g) {
        if(v[name].comp_id == -1) {
            Component c;
            int comp_id = 0;
            c.value = dfs(g, c.names, v, c.cutpoints, name, comp_id);
            ++comp_id;
            comps.push_back(c);
        }
    }
}

//Объединение смежных вершин, не являющихся точками сочленения, в одну.
void reduce_comp(Graph &g, Values &v, Component &c, std::string predator) {
    value_t value = 0;
    
    std::list<std::string> to_check(g[predator].begin(), g[predator].end());

    for(auto it = to_check.begin(); it != to_check.end();) {
        auto victim = *it;
        auto next = std::next(it);
        if(!v[victim].is_cutp && !v[victim].is_reduced) {
            value += v[victim].value;
            for(auto future_victim : g[victim]) {
                if(future_victim != predator) {
                    g[predator].insert(future_victim);
                    to_check.push_back(future_victim);
                    g[future_victim].erase(victim);
                    g[future_victim].insert(predator);
                }
            }
            g[predator].erase(victim);
            g.erase(victim);
            v[victim].is_reduced = true;
        }
        it = next;
    }
    v[predator].sum = v[predator].value + value;
}

//Объединение всех групп смежных точек, не являющихся точками сочленения в одну (на каждую группу)
void reduce(Graph &g, Values &v, Component &c) {
    for(auto name : c.names) {
        if(!v[name].is_cutp && !v[name].is_reduced) {
            reduce_comp(g, v, c, name);
        }
    }
}

//Подсчет эффективности подсети
value_t count_graph(Graph &g, Values &v, Component &c, std::set<std::string> &used,
        std::string name) {
    value_t counter = 0;
    used.insert(name);
    for(auto link : g[name]) {
        if(used.find(link) == used.end()) {
            counter += count_graph(g, v, c, used, link);
        }
    }
    return counter + v[name].sum;
}

//Отладочная печать графа и вершин
void print_gnv(Graph &g, Values &v) {
    std::cout << "Graph:" << std::endl;
    for(auto &[name, links] : g) {
        std::cout << name << ": ";
        for(auto link : links) {
            std::cout << link << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "Nodes:" << std::endl;
    for(auto &[name, node] : v) {
        std::cout << name << " - " << node.value << std::endl;
    }
}

//Отладочная печать компонент связности
void print_comps(Components &comps) {
    int i = 0;
    for(auto comp : comps) {
        std::cout << "comp " << i << ", value " << comp.value << ": ";
        for(auto name : comp.names) {
            std::cout << name << " ";
        }
        std::cout << std::endl;
        ++i;
    }
}

//Отладочная печать всех точек сочленения
void print_cutps(Values &v) {
    std::cout << "cutpoints: ";
    for(auto &[name, node] : v) {
        if(node.is_cutp) {
            std::cout << name << " ";
        }
    }
    std::cout << std::endl;

}

int main(int argc, char *argv[]) {
    if(argc < 2) {
        std::cout << "Usage: mgt file1 [file2 [...]]"
                        << std::endl;
        return EXIT_FAILURE;
    }

    for(int i = 1; i < argc; ++i) {
        Graph graph;
        Values values;
        std::map<std::string, value_t> variants;
        auto min_vitality = std::numeric_limits<value_t>::max();

        //Отладка
        std::cout <<argv[i] <<": ";

        std::ifstream in(argv[i]);
        if(!in.is_open()) {
            std::cout <<"can't open file" <<std::endl;
            continue;
        }

        parse(graph, values, in, std::cout);

        Components comps;
        make_components(graph, values, comps);
        
        for(auto c : comps) {
            reduce(graph, values, c);
        }

        for(auto &[name, links] : graph) {
            if(values[name].is_cutp) {
                values[name].sum = values[name].value;
            }
        }
        
        for(auto &[name, node] : values) {
            value_t vitality = 0;
            int comp_id = 0;
            for(auto comp : comps) {
                if(comp_id == node.comp_id) {
                    value_t result;
                    if(!node.is_cutp) {
                        result = comp.value - node.value;
                        vitality += result * result;
                    } else {
                        std::set<std::string> used;
                        used.insert(name);
                        result = 0;
                        for(auto link : graph[name]) {
                            if(used.find(link) == used.end()) {
                                result = count_graph(graph, values, comp, used, link);
                                vitality += result * result;
                            }
                        }
                    }
                } else {
                    vitality += comp.value * comp.value;
                }
                ++comp_id;
            }
            vitality += values[name].value;
            variants[name] = vitality;
            if(vitality < min_vitality) {
                min_vitality = vitality;
            }
        }


        std::cout << "[";
        bool is_only_answer = true;
        for(auto& [name, vitality] : variants) {
            if(vitality == min_vitality) {
                if(is_only_answer) {
                    std::cout << "\'" << name << "\'";
                    is_only_answer = false;
                } else {
                    std::cout << ", \'" << name << "\'";
                }
            }
        }
        std::cout << "]" << std::endl;
    }
    return 0;
}
