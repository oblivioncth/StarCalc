#ifndef RANK_H
#define RANK_H

// Qt Includes
#include <QSet>

namespace Star
{

struct Rank
{
    enum Order { Ascending, Descending };

    int value;
    QSet<QString> candidates;

    static QList<Rank> rankSort(const QMap<QString, int>& valueMap, Order order = Descending);
};

}

#endif // RANK_H
