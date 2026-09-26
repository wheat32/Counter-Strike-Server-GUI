#include "mapBrowserDialog.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QStyle>
#include <QTableWidget>
#include <QVBoxLayout>

#include "../mapTypes.h"

namespace
{
constexpr int DIALOG_MARGIN         = 20;
constexpr int DIALOG_SPACING        = 12;
constexpr int DIALOG_DEFAULT_WIDTH  = 640;
constexpr int DIALOG_DEFAULT_HEIGHT = 480;
constexpr int COLUMN_SPACING        = 8;
constexpr int FRAME_SIDES           = 2; // left + right
} // namespace

MapBrowserDialog::MapBrowserDialog(const QStringList& maps, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("All Maps"));
    resize(DIALOG_DEFAULT_WIDTH, DIALOG_DEFAULT_HEIGHT);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(DIALOG_MARGIN, DIALOG_MARGIN, DIALOG_MARGIN, DIALOG_MARGIN);
    layout->setSpacing(DIALOG_SPACING);

    QLabel* hintLabel = new QLabel(
        tr("Select the map the server loads on startup. Double-click a map to choose it."), this);
    hintLabel->setWordWrap(true);
    layout->addWidget(hintLabel);

    QHBoxLayout* columnsLayout = new QHBoxLayout;
    columnsLayout->setSpacing(COLUMN_SPACING);
    for (const auto& [type, members] : MapTypes::groupByType(maps))
    {
        QTableWidget* column = makeColumn(MapTypes::typeLabel(type), members);
        m_columns.append(column);
        columnsLayout->addWidget(column, 1);
    }
    layout->addLayout(columnsLayout, 1);

    // Set the accent property before the box adds (and styles) the button.
    m_chooseBtn = new QPushButton(tr("Choose"), this);
    m_chooseBtn->setProperty("accent", true);
    QDialogButtonBox* box = new QDialogButtonBox(this);
    box->addButton(m_chooseBtn, QDialogButtonBox::AcceptRole);
    box->addButton(QDialogButtonBox::Cancel);
    connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(box);

    updateChooseEnabled();
}

QTableWidget* MapBrowserDialog::makeColumn(const QString& header, const QStringList& maps)
{
    QTableWidget* column = new QTableWidget(static_cast<int>(maps.size()), 1, this);
    column->setObjectName(QStringLiteral("mapBrowserColumn"));

    // The QTableWidget constructor creates and styles the header before the
    // table has a name, so re-polish it for "#mapBrowserColumn QHeaderView"
    // style sheet rules to apply.
    QHeaderView* headerView = column->horizontalHeader();
    headerView->style()->unpolish(headerView);
    headerView->style()->polish(headerView);

    column->setHorizontalHeaderItem(0, new QTableWidgetItem(header));
    column->setEditTriggers(QAbstractItemView::NoEditTriggers);
    column->setSelectionMode(QAbstractItemView::SingleSelection);
    column->setSelectionBehavior(QAbstractItemView::SelectItems);
    column->setShowGrid(false);
    column->setWordWrap(false);
    column->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    column->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    column->verticalHeader()->setVisible(false);
    headerView->setSectionsClickable(false);

    for (int row = 0; row < maps.size(); ++row)
    {
        QTableWidgetItem* item = new QTableWidgetItem(maps.at(row));
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        column->setItem(row, 0, item);
    }

    // Wide enough for the longest name or the header, plus room for a
    // scrollbar, so nothing is cut off when one appears. Extra width stretches.
    column->resizeColumnsToContents();
    const int scrollBarWidth = column->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    column->setMinimumWidth(headerView->sectionSize(0) + scrollBarWidth
                            + FRAME_SIDES * column->frameWidth());
    headerView->setSectionResizeMode(QHeaderView::Stretch);

    connect(column, &QTableWidget::itemSelectionChanged, this, [this, column]()
    {
        onColumnSelectionChanged(column);
    });
    connect(column, &QTableWidget::itemDoubleClicked, this, &QDialog::accept);

    return column;
}

void MapBrowserDialog::onColumnSelectionChanged(const QTableWidget* column)
{
    if (column->selectedItems().isEmpty() == false)
    {
        for (QTableWidget* other : std::as_const(m_columns))
        {
            if (other != column)
            {
                other->clearSelection();
            }
        }
    }
    updateChooseEnabled();
}

QString MapBrowserDialog::selectedMap() const
{
    for (const QTableWidget* column : m_columns)
    {
        const QList<QTableWidgetItem*> selected = column->selectedItems();
        if (selected.isEmpty() == false)
        {
            return selected.first()->text();
        }
    }
    return QString();
}

void MapBrowserDialog::updateChooseEnabled() const
{
    m_chooseBtn->setEnabled(selectedMap().isEmpty() == false);
}
