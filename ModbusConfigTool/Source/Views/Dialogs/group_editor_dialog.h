#ifndef GROUP_EDITOR_DIALOG_H
#define GROUP_EDITOR_DIALOG_H

#include "Domain/Models/register_group.h"

#include <QDialog>

class QLabel;
class QLineEdit;
class QSlider;
class QTextEdit;
class ColorWheelWidget;

class GroupEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GroupEditorDialog(const RegisterGroup &group, QWidget *parent = nullptr);

    RegisterGroup group() const;

private:
    void onColorPicked(const QColor &color);
    void updateColorPreview();

    RegisterGroup m_group;
    QLineEdit *m_nameEdit = nullptr;
    QTextEdit *m_descEdit = nullptr;
    ColorWheelWidget *m_colorWheel = nullptr;
    QSlider *m_valueSlider = nullptr;
    QLabel *m_colorPreview = nullptr;
    QLabel *m_colorValueLabel = nullptr;
    QString m_selectedColor;
};

#endif
