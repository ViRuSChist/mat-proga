#pragma once

// Чтение и запись графа в текстовый файл.
// Формат: первая значимая строка — число узлов. Далее по одному ребру
// в строке: номер исходного узла, номер конечного узла, вес.
// Строки, начинающиеся с '#', и пустые строки игнорируются.

#include "graph.h"
#include <QString>

class GraphIO
{
public:
    static bool loadFromFile(const QString &path, Graph &outGraph, QString &errorMsg);
    static bool saveToFile(const QString &path, const Graph &graph, QString &errorMsg);
};

