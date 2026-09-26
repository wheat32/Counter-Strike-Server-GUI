#pragma once

#include <QDialog>
#include <QList>
#include <QStringList>

class QPushButton;
class QTableWidget;

// Shows every map, one column per map type (as_, cs_, de_, ...), and lets the
// user pick one. Each column scrolls on its own. Choose stays disabled until a
// map is selected.
class MapBrowserDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MapBrowserDialog(const QStringList& maps, QWidget* parent = nullptr);

    // The selected map, or an empty string if none is selected.
    [[nodiscard]] QString selectedMap() const;

private:
    // Creates one scrollable, single-column list of maps under a type header.
    QTableWidget* makeColumn(const QString& header, const QStringList& maps);

    // Keeps the selection to one map across all columns.
    void onColumnSelectionChanged(const QTableWidget* column);

    void updateChooseEnabled() const;

    QList<QTableWidget*> m_columns;
    QPushButton*         m_chooseBtn = nullptr;
};
