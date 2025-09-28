#pragma once

#include <QtWidgets>
#include "roi.hpp"


// Forward declaration
class PropertyItemDelegate;


class PropertyBrowser : public QWidget
{
    Q_OBJECT

public:
    explicit PropertyBrowser(QWidget *parent = nullptr);
    ~PropertyBrowser() override;

    void setRoi(Roi* roi);
    PropertyInfo getPropertyInfo(QTreeWidgetItem* item) const;

signals:
    void propertyValueChanged();

public slots:
    void onRoiSelectionChanged(Roi* roi);

private slots:
    void onItemChanged(QTreeWidgetItem *item, int column);
    void onEditorValueChanged();
    void onItemClicked(QTreeWidgetItem* item, int column);

private:
    QTreeWidget* m_treeWidget;
    Roi*         m_currentRoi;
    bool         m_updatingFromCode;  // Flag to prevent recursion when updating from code
    PropertyItemDelegate* m_delegate;

    void updateProperties();
    void addPropertySection(PropertyAdapter* adapter);
    QTreeWidgetItem* createPropertyItem(const PropertyInfo& propInfo, PropertyAdapter* adapter);
    void updateItemValue(QTreeWidgetItem* item, const QVariant& value, const PropertyInfo& propInfo);
    QVariant getItemValue(QTreeWidgetItem* item, const PropertyInfo& propInfo);
    void updatePropertyValue(QTreeWidgetItem* item, const QVariant& value);

    friend class PropertyItemDelegate;
    friend class PropertyTreeWidget;
};
