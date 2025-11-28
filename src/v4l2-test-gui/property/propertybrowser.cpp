#include "propertybrowser.hpp"


// Custom SpinBox with improved mouse wheel behavior and immediate value commit on step changes
class CustomSpinBox : public QSpinBox
{
    Q_OBJECT
    
public:
    explicit CustomSpinBox(QWidget* parent = nullptr) : 
        QSpinBox(parent) 
    {
        setLocale(QLocale::system());
    }
    
signals:
    void valueChangedByStep(int value);
    
protected:
    void wheelEvent(QWheelEvent* event) override
    {
        // Make mouse wheel more responsive by treating each wheel step as a single increment
        if (event->angleDelta().y() > 0) {
            stepBy(1);
        } else if (event->angleDelta().y() < 0) {
            stepBy(-1);
        }
        event->accept();
    }
    
    void stepBy(int steps) override
    {
        // Call base implementation to change the value
        QSpinBox::stepBy(steps);
        
        // Emit signal to notify about step-based value change (wheel or arrow keys)
        emit valueChangedByStep(value());
    }
};

// Custom DoubleSpinBox with improved mouse wheel behavior and immediate value commit on step changes
class CustomDoubleSpinBox : public QDoubleSpinBox
{
    Q_OBJECT
    
public:
    explicit CustomDoubleSpinBox(QWidget* parent = nullptr) : 
        QDoubleSpinBox(parent) 
    {
        setLocale(QLocale::system());
    }
    
signals:
    void valueChangedByStep(double value);
    
protected:
    void wheelEvent(QWheelEvent* event) override
    {
        // Make mouse wheel more responsive by treating each wheel step as a single increment
        if (event->angleDelta().y() > 0) {
            stepBy(1);
        } else if (event->angleDelta().y() < 0) {
            stepBy(-1);
        }
        event->accept();
    }
    
    void stepBy(int steps) override
    {
        // Call base implementation to change the value
        QDoubleSpinBox::stepBy(steps);
        
        // Emit signal to notify about step-based value change (wheel or arrow keys)
        emit valueChangedByStep(value());
    }
};

// Custom TreeWidget to handle mouse clicks for immediate editing
class PropertyTreeWidget : public QTreeWidget
{
    Q_OBJECT
    
public:
    explicit PropertyTreeWidget(PropertyBrowser* browser, QWidget* parent = nullptr) :
        QTreeWidget(parent), m_browser(browser)
    {
    }
    
protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        // Call base implementation first
        QTreeWidget::mousePressEvent(event);
        
        // Get the item that was clicked
        QTreeWidgetItem* item = itemAt(event->pos());
        if (!item) {
            return;
        }
        
        // Check if we clicked in the value column
        int column = columnAt(event->pos().x());
        if (column != 1) {
            return;
        }
        
        // Get property info
        PropertyInfo propInfo = m_browser->getPropertyInfo(item);
        if (propInfo.name.isEmpty()) {
            return;
        }
        
        // For non-bool types, open editor immediately on single click
        if (propInfo.type != QMetaType::Bool) {
            // Start editing if not already editing
            if (state() != QAbstractItemView::EditingState) {
                edit(currentIndex());
            }
        }
    }
    
private:
    PropertyBrowser* m_browser;
};

// Custom delegate for property editing
class PropertyItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit PropertyItemDelegate(PropertyBrowser* browser, QObject* parent = nullptr)
        : QStyledItemDelegate(parent), m_browser(browser)
    {
    }

    // Helper function to get PropertyInfo from adapter
    PropertyInfo getPropertyInfo(QTreeWidgetItem* item) const
    {
        if (!item) {
            return PropertyInfo();
        }

        QString propertyName = item->data(0, Qt::UserRole).toString();
        PropertyAdapter* adapter = static_cast<PropertyAdapter*>(item->data(0, Qt::UserRole + 1).value<void*>());
        
        if (!adapter || propertyName.isEmpty()) {
            return PropertyInfo();
        }

        // Find the PropertyInfo for this property
        QList<PropertyInfo> properties = adapter->availableProperties();
        for (const PropertyInfo& propInfo : properties) {
            if (propInfo.name == propertyName) {
                return propInfo;
            }
        }

        return PropertyInfo();
    }

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        QTreeWidgetItem* item = m_browser->m_treeWidget->itemFromIndex(index);
        if (!item || index.column() != 1) {
            return QStyledItemDelegate::createEditor(parent, option, index);
        }

        // Get PropertyInfo from adapter
        PropertyInfo propInfo = getPropertyInfo(item);
        if (propInfo.name.isEmpty()) {
            return QStyledItemDelegate::createEditor(parent, option, index);
        }

        switch (propInfo.type) {
            case QMetaType::Int: {
                CustomSpinBox* spinBox = new CustomSpinBox(parent);
                spinBox->setMinimum(propInfo.min.isValid() ? propInfo.min.toInt() : std::numeric_limits<int>::min());
                spinBox->setMaximum(propInfo.max.isValid() ? propInfo.max.toInt() : std::numeric_limits<int>::max());
                spinBox->setSingleStep(propInfo.step.isValid() ? propInfo.step.toInt() : 1);
                spinBox->setFrame(false);
                
                // Connect step-based value changes (mouse wheel, arrow keys) for immediate commit
                connect(spinBox, &CustomSpinBox::valueChangedByStep, [this, spinBox]() {
                    // Find the item being edited and commit the value immediately
                    QTreeWidgetItem* currentItem = m_browser->m_treeWidget->currentItem();
                    if (currentItem) {
                        m_browser->updatePropertyValue(currentItem, spinBox->value());
                    }
                });
                
                return spinBox;
            }

            case QMetaType::Double: {
                CustomDoubleSpinBox* spinBox = new CustomDoubleSpinBox(parent);
                spinBox->setMinimum(propInfo.min.isValid() ? propInfo.min.toDouble() : std::numeric_limits<double>::lowest());
                spinBox->setMaximum(propInfo.max.isValid() ? propInfo.max.toDouble() : std::numeric_limits<double>::max());
                spinBox->setSingleStep(propInfo.step.isValid() ? propInfo.step.toDouble() : 0.1);
                spinBox->setDecimals(3);
                spinBox->setFrame(false);
                
                // Connect step-based value changes (mouse wheel, arrow keys) for immediate commit
                connect(spinBox, &CustomDoubleSpinBox::valueChangedByStep, [this, spinBox]() {
                    // Find the item being edited and commit the value immediately
                    QTreeWidgetItem* currentItem = m_browser->m_treeWidget->currentItem();
                    if (currentItem) {
                        m_browser->updatePropertyValue(currentItem, spinBox->value());
                    }
                });
                
                return spinBox;
            }

            case QMetaType::Bool: {
                QCheckBox* checkBox = new QCheckBox(parent);
                
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
                connect(checkBox, &QCheckBox::checkStateChanged, 
                        m_browser, &PropertyBrowser::onEditorValueChanged);
#else
                connect(checkBox, &QCheckBox::stateChanged, 
                        m_browser, &PropertyBrowser::onEditorValueChanged);
#endif
                
                return checkBox;
            }

            default:
                return QStyledItemDelegate::createEditor(parent, option, index);
        }
    }

    void setEditorData(QWidget* editor, const QModelIndex& index) const override
    {
        QTreeWidgetItem* item = m_browser->m_treeWidget->itemFromIndex(index);
        if (!item || index.column() != 1) {
            return QStyledItemDelegate::setEditorData(editor, index);
        }

        PropertyInfo propInfo = getPropertyInfo(item);
        if (propInfo.name.isEmpty()) {
            return;
        }

        PropertyAdapter* adapter = static_cast<PropertyAdapter*>(item->data(0, Qt::UserRole + 1).value<void*>());
        if (!adapter) {
            return;
        }

        QVariant value = adapter->getValue(propInfo.name);

        switch (propInfo.type) {
            case QMetaType::Int:
                if (QSpinBox* spinBox = qobject_cast<QSpinBox*>(editor)) {
                    spinBox->setValue(value.toInt());
                    spinBox->selectAll();
                }
                break;

            case QMetaType::Double:
                if (QDoubleSpinBox* spinBox = qobject_cast<QDoubleSpinBox*>(editor)) {
                    spinBox->setValue(value.toDouble());
                    spinBox->selectAll();
                }
                break;

            case QMetaType::Bool:
                if (QCheckBox* checkBox = qobject_cast<QCheckBox*>(editor)) {
                    checkBox->setChecked(value.toBool());
                }
                break;

            default:
                QStyledItemDelegate::setEditorData(editor, index);
                break;
        }
    }

    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override
    {
        QTreeWidgetItem* item = m_browser->m_treeWidget->itemFromIndex(index);
        if (!item || index.column() != 1) {
            return QStyledItemDelegate::setModelData(editor, model, index);
        }

        PropertyInfo propInfo = getPropertyInfo(item);
        if (propInfo.name.isEmpty()) {
            return;
        }

        QVariant newValue;

        switch (propInfo.type) {
            case QMetaType::Int:
                if (QSpinBox* spinBox = qobject_cast<QSpinBox*>(editor)) {
                    newValue = spinBox->value();
                }
                break;

            case QMetaType::Double:
                if (QDoubleSpinBox* spinBox = qobject_cast<QDoubleSpinBox*>(editor)) {
                    newValue = spinBox->value();
                }
                break;

            case QMetaType::Bool:
                if (QCheckBox* checkBox = qobject_cast<QCheckBox*>(editor)) {
                    newValue = checkBox->isChecked();
                }
                break;

            default:
                QStyledItemDelegate::setModelData(editor, model, index);
                return;
        }

        if (newValue.isValid()) {
            m_browser->updatePropertyValue(item, newValue);
        }
    }

    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        QTreeWidgetItem* item = m_browser->m_treeWidget->itemFromIndex(index);
        if (item && index.column() == 1) {
            PropertyInfo propInfo = getPropertyInfo(item);
            
            // Left-align checkbox in the cell without text
            if (propInfo.type == QMetaType::Bool) {
                if (QCheckBox* checkBox = qobject_cast<QCheckBox*>(editor)) {
                    QRect rect = option.rect;
                    checkBox->setText("");  // Remove any text
                    checkBox->setGeometry(rect.left() + 2, rect.top(), rect.width(), rect.height());
                    return;
                }
            }
        }
        
        editor->setGeometry(option.rect);
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        QTreeWidgetItem* item = m_browser->m_treeWidget->itemFromIndex(index);
        if (item && index.column() == 1) {
            PropertyInfo propInfo = getPropertyInfo(item);
            
            // For bool type, draw checkbox when not editing
            if (propInfo.type == QMetaType::Bool) {
                QStyleOptionButton checkBoxOption;
                checkBoxOption.state |= QStyle::State_Enabled;
                
                // Get the stored bool value
                bool checked = item->data(1, Qt::UserRole).toBool();
                if (checked) {
                    checkBoxOption.state |= QStyle::State_On;
                } else {
                    checkBoxOption.state |= QStyle::State_Off;
                }
                
                // Left-align the checkbox with small margin
                int checkBoxWidth = QApplication::style()->pixelMetric(QStyle::PM_IndicatorWidth);
                int checkBoxHeight = QApplication::style()->pixelMetric(QStyle::PM_IndicatorHeight);
                int x = option.rect.left() + 2;  // Small left margin
                int y = option.rect.top() + (option.rect.height() - checkBoxHeight) / 2;
                
                checkBoxOption.rect = QRect(x, y, checkBoxWidth, checkBoxHeight);
                QApplication::style()->drawControl(QStyle::CE_CheckBox, &checkBoxOption, painter);
                return;
            }
        }
        
        QStyledItemDelegate::paint(painter, option, index);
    }

private:
    PropertyBrowser* m_browser;
};


PropertyBrowser::PropertyBrowser(QWidget *parent)
    : QWidget(parent),
      m_currentRoi(nullptr),
      m_updatingFromCode(false)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    m_treeWidget = new PropertyTreeWidget(this, this);
    m_treeWidget->setHeaderLabels({tr("Property"), tr("Value")});
    m_treeWidget->setColumnCount(2);
    m_treeWidget->setAlternatingRowColors(true);
    m_treeWidget->setIndentation(20);
    m_treeWidget->setColumnWidth(0, 150);
    
    // Enable editing with key press
    m_treeWidget->setEditTriggers(QAbstractItemView::AnyKeyPressed |
                                   QAbstractItemView::EditKeyPressed);
    
    // Install custom delegate for property editing
    m_delegate = new PropertyItemDelegate(this, this);
    m_treeWidget->setItemDelegateForColumn(1, m_delegate);
    
    layout->addWidget(m_treeWidget);
    
    connect(m_treeWidget, &QTreeWidget::itemChanged, this, &PropertyBrowser::onItemChanged);
    connect(m_treeWidget, &QTreeWidget::itemClicked, this, &PropertyBrowser::onItemClicked);
}

PropertyBrowser::~PropertyBrowser()
{
}

void PropertyBrowser::onRoiSelectionChanged(Roi* roi)
{
    if (roi != nullptr) {
        m_currentRoi = roi;
        updateProperties();

    } else {
        m_currentRoi = nullptr;
        m_treeWidget->clear();
    }
}

void PropertyBrowser::updateProperties()
{
    m_updatingFromCode = true;
    m_treeWidget->clear();
    
    if (!m_currentRoi) {
        m_updatingFromCode = false;
        return;
    }
    
    // Add ROI properties section
    PropertyAdapter* roiAdapter = m_currentRoi->propertyAdapter();
    if (roiAdapter) {
        addPropertySection(roiAdapter);
    }
    
    // Add function properties sections
    const auto& functions = m_currentRoi->getFunctions();
    for (const auto& function : functions) {
        if (function) {
            PropertyAdapter* funcAdapter = function->propertyAdapter();
            if (funcAdapter) {
                addPropertySection(funcAdapter);
            }
        }
    }
    
    m_treeWidget->expandAll();
    m_updatingFromCode = false;
}

void PropertyBrowser::addPropertySection(PropertyAdapter* adapter)
{
    if (!adapter) {
        return;  // Safety check
    }
    
    QTreeWidgetItem* sectionItem = new QTreeWidgetItem(m_treeWidget);
    sectionItem->setText(0, adapter->sectionName());
    sectionItem->setFirstColumnSpanned(true);
    sectionItem->setFlags(Qt::ItemIsEnabled);
    
    // Set bold font for section headers
    QFont font = sectionItem->font(0);
    font.setBold(true);
    sectionItem->setFont(0, font);
    
    // Add properties
    QList<PropertyInfo> properties = adapter->availableProperties();
    for (const PropertyInfo& propInfo : properties) {
        QTreeWidgetItem* propertyItem = createPropertyItem(propInfo, adapter);
        if (propertyItem) {
            sectionItem->addChild(propertyItem);
        }
    }
}

QTreeWidgetItem* PropertyBrowser::createPropertyItem(const PropertyInfo& propInfo, PropertyAdapter* adapter)
{
    if (!adapter) {
        return nullptr;  // Safety check
    }
    
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setText(0, propInfo.name);
    item->setData(0, Qt::UserRole, propInfo.name);  // Store property name
    item->setData(0, Qt::UserRole + 1, QVariant::fromValue((void*)adapter));  // Store adapter pointer
    
    // Get current value from adapter
    QVariant value = adapter->getValue(propInfo.name);
    updateItemValue(item, value, propInfo);
    
    return item;
}

void PropertyBrowser::updateItemValue(QTreeWidgetItem* item, const QVariant& value, const PropertyInfo& propInfo)
{
    QString displayText;
    
    switch (propInfo.type) {
        case QMetaType::Int:
            displayText = QString::number(value.toInt());
            if (!propInfo.unit.isEmpty()) {
                displayText += " " + propInfo.unit;
            }
            item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
            break;
            
        case QMetaType::Double:
            displayText = QString::number(value.toDouble(), 'f', 3);
            if (!propInfo.unit.isEmpty()) {
                displayText += " " + propInfo.unit;
            }
            item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
            break;
            
        case QMetaType::Bool:
            // For bool values, don't show text - the checkbox itself is sufficient
            displayText = "";
            item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
            // Store the actual bool value in UserRole for the delegate to read
            item->setData(1, Qt::UserRole, value.toBool());
            break;
            
        case QMetaType::QString:
            displayText = value.toString();
            item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
            break;
            
        default:
            displayText = value.toString();
            item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            break;
    }
    
    item->setText(1, displayText);
}

QVariant PropertyBrowser::getItemValue(QTreeWidgetItem* item, const PropertyInfo& propInfo)
{
    QString text = item->text(1);
    
    // Remove unit suffix if present
    if (!propInfo.unit.isEmpty() && text.endsWith(propInfo.unit)) {
        text = text.left(text.length() - propInfo.unit.length()).trimmed();
    }
    
    switch (propInfo.type) {
        case QMetaType::Int:
            return text.toInt();
            
        case QMetaType::Double:
            return text.toDouble();
            
        case QMetaType::Bool:
            return item->checkState(1) == Qt::Checked;
            
        case QMetaType::QString:
            return text;
            
        default:
            return text;
    }
}

void PropertyBrowser::onItemChanged(QTreeWidgetItem *item, int column)
{
    if (m_updatingFromCode || !item || column != 1) {
        return;
    }
    
    // Get property info and adapter
    PropertyInfo propInfo = getPropertyInfo(item);
    if (propInfo.name.isEmpty()) {
        return;
    }
    
    PropertyAdapter* adapter = static_cast<PropertyAdapter*>(item->data(0, Qt::UserRole + 1).value<void*>());
    if (!adapter) {
        return;
    }
    
    // Get new value from item
    QVariant newValue = getItemValue(item, propInfo);
    
    // Validate range
    if (propInfo.type == QMetaType::Int || propInfo.type == QMetaType::Double) {
        double val = newValue.toDouble();
        double minVal = propInfo.min.toDouble();
        double maxVal = propInfo.max.toDouble();
        
        if (val < minVal) {
            newValue = propInfo.min;
        } else if (val > maxVal) {
            newValue = propInfo.max;
        }
    }
    
    // Set value through adapter
    adapter->setValue(propInfo.name, newValue);
    
    // Update display with validated value
    m_updatingFromCode = true;
    QVariant actualValue = adapter->getValue(propInfo.name);
    updateItemValue(item, actualValue, propInfo);
    m_updatingFromCode = false;
}

void PropertyBrowser::onEditorValueChanged()
{
    // Get the current item being edited
    QTreeWidgetItem* currentItem = m_treeWidget->currentItem();
    if (!currentItem) {
        return;
    }

    // Get the editor widget
    QWidget* editor = m_treeWidget->itemWidget(currentItem, 1);
    if (!editor) {
        // For persistent editors or during editing, get the editor from the viewport
        QWidget* viewport = m_treeWidget->viewport();
        editor = viewport->findChild<QSpinBox*>();
        if (!editor) {
            editor = viewport->findChild<QDoubleSpinBox*>();
        }
        if (!editor) {
            editor = viewport->findChild<QCheckBox*>();
        }
    }
    
    if (!editor) {
        return;
    }

    // Get property info
    PropertyInfo propInfo = getPropertyInfo(currentItem);
    if (propInfo.name.isEmpty()) {
        return;
    }

    PropertyAdapter* adapter = static_cast<PropertyAdapter*>(currentItem->data(0, Qt::UserRole + 1).value<void*>());
    if (!adapter) {
        return;
    }

    QVariant newValue;

    // Get value from editor
    switch (propInfo.type) {
        case QMetaType::Int:
            if (QSpinBox* spinBox = qobject_cast<QSpinBox*>(editor)) {
                newValue = spinBox->value();
            }
            break;

        case QMetaType::Double:
            if (QDoubleSpinBox* spinBox = qobject_cast<QDoubleSpinBox*>(editor)) {
                newValue = spinBox->value();
            }
            break;

        case QMetaType::Bool:
            if (QCheckBox* checkBox = qobject_cast<QCheckBox*>(editor)) {
                newValue = checkBox->isChecked();
            }
            break;

        default:
            return;
    }

    if (newValue.isValid()) {
        updatePropertyValue(currentItem, newValue);
    }
}

void PropertyBrowser::updatePropertyValue(QTreeWidgetItem* item, const QVariant& value)
{
    if (!item) {
        return;
    }

    PropertyInfo propInfo = getPropertyInfo(item);
    if (propInfo.name.isEmpty()) {
        return;
    }
    
    PropertyAdapter* adapter = static_cast<PropertyAdapter*>(item->data(0, Qt::UserRole + 1).value<void*>());
    if (!adapter) {
        return;
    }

    // Set value through adapter
    m_updatingFromCode = true;
    adapter->setValue(propInfo.name, value);
    
    // Update display with actual value (may be clamped/validated by adapter)
    QVariant actualValue = adapter->getValue(propInfo.name);
    updateItemValue(item, actualValue, propInfo);
    
    m_updatingFromCode = false;
    
    // Emit signal to notify that a property value has changed
    emit propertyValueChanged();
}

PropertyInfo PropertyBrowser::getPropertyInfo(QTreeWidgetItem* item) const
{
    if (!item) {
        return PropertyInfo();
    }

    QString propertyName = item->data(0, Qt::UserRole).toString();
    PropertyAdapter* adapter = static_cast<PropertyAdapter*>(item->data(0, Qt::UserRole + 1).value<void*>());
    
    if (!adapter || propertyName.isEmpty()) {
        return PropertyInfo();
    }

    // Find the PropertyInfo for this property from the adapter
    QList<PropertyInfo> properties = adapter->availableProperties();
    for (const PropertyInfo& propInfo : properties) {
        if (propInfo.name == propertyName) {
            return propInfo;
        }
    }

    return PropertyInfo();
}

void PropertyBrowser::onItemClicked(QTreeWidgetItem* item, int column)
{
    if (!item || column != 1 || m_updatingFromCode) {
        return;
    }

    PropertyInfo propInfo = getPropertyInfo(item);
    if (propInfo.name.isEmpty()) {
        return;
    }
    
    // For boolean properties, toggle value on single click
    if (propInfo.type == QMetaType::Bool) {
        PropertyAdapter* adapter = static_cast<PropertyAdapter*>(item->data(0, Qt::UserRole + 1).value<void*>());
        if (!adapter) {
            return;
        }
        
        // Toggle the value
        bool currentValue = adapter->getValue(propInfo.name).toBool();
        updatePropertyValue(item, !currentValue);
    }
}

#include "propertybrowser.moc"