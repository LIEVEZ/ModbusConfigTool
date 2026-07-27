#include "group_editor_dialog.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QPainter>
#include <QSlider>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QtMath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <functional>

namespace
{
QString colorToHex(const QColor &color)
{
    return color.name(QColor::HexRgb);
}
}

// 圆形 HSV 色盘：角度=色相，半径=饱和度；外部滑条控制明度
class ColorWheelWidget : public QWidget
{
public:
    explicit ColorWheelWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setObjectName(QStringLiteral("colorWheelWidget"));
        setFixedSize(176, 176);
        setMouseTracking(true);
        setCursor(Qt::CrossCursor);
    }

    QColor color() const
    {
        return QColor::fromHsvF(m_hue, m_saturation, m_value);
    }

    void setColor(const QColor &color)
    {
        if (!color.isValid())
        {
            return;
        }
        qreal h = 0.0;
        qreal s = 0.0;
        qreal v = 0.0;
        color.getHsvF(&h, &s, &v);
        if (h < 0.0)
        {
            h = m_hue;
        }
        m_hue = h;
        m_saturation = s;
        m_value = v;
        update();
        if (m_onColorChanged)
        {
            m_onColorChanged(this->color());
        }
    }

    void setValue(qreal value)
    {
        m_value = qBound(0.0, value, 1.0);
        update();
        if (m_onColorChanged)
        {
            m_onColorChanged(color());
        }
    }

    qreal value() const { return m_value; }

    void setColorChangedCallback(const std::function<void(const QColor &)> &callback)
    {
        m_onColorChanged = callback;
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const QRectF wheelRect = wheelGeometry();
        const QPointF center = wheelRect.center();
        const qreal radius = wheelRect.width() / 2.0;

        QImage image(qCeil(wheelRect.width()), qCeil(wheelRect.height()),
                     QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        for (int y = 0; y < image.height(); ++y)
        {
            auto *line = reinterpret_cast<QRgb *>(image.scanLine(y));
            for (int x = 0; x < image.width(); ++x)
            {
                const qreal dx = x + 0.5 - radius;
                const qreal dy = y + 0.5 - radius;
                const qreal dist = qSqrt(dx * dx + dy * dy);
                if (dist > radius)
                {
                    line[x] = qRgba(0, 0, 0, 0);
                    continue;
                }
                qreal hue = qAtan2(dy, dx) / (2.0 * M_PI);
                if (hue < 0.0)
                {
                    hue += 1.0;
                }
                const qreal sat = qBound(0.0, dist / radius, 1.0);
                line[x] = QColor::fromHsvF(hue, sat, m_value).rgba();
            }
        }
        painter.drawImage(wheelRect.topLeft(), image);

        painter.setPen(QPen(QColor(38, 37, 30, 45), 1.0));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(wheelRect.adjusted(0.5, 0.5, -0.5, -0.5));

        const qreal markerAngle = m_hue * 2.0 * M_PI;
        const qreal markerRadius = m_saturation * radius;
        const QPointF marker(
            center.x() + qCos(markerAngle) * markerRadius,
            center.y() + qSin(markerAngle) * markerRadius);

        painter.setPen(QPen(Qt::white, 2.0));
        painter.setBrush(color());
        painter.drawEllipse(marker, 7.0, 7.0);
        painter.setPen(QPen(QColor(38, 37, 30, 170), 1.0));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(marker, 7.0, 7.0);
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton)
        {
            pickAt(event->pos());
            event->accept();
            return;
        }
        QWidget::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (event->buttons() & Qt::LeftButton)
        {
            pickAt(event->pos());
            event->accept();
            return;
        }
        QWidget::mouseMoveEvent(event);
    }

private:
    QRectF wheelGeometry() const
    {
        const qreal size = qMin(width() - 4.0, height() - 4.0);
        const qreal x = (width() - size) / 2.0;
        const qreal y = (height() - size) / 2.0;
        return QRectF(x, y, size, size);
    }

    void pickAt(const QPoint &pos)
    {
        const QRectF wheelRect = wheelGeometry();
        const QPointF center = wheelRect.center();
        const qreal radius = wheelRect.width() / 2.0;
        const qreal dx = pos.x() - center.x();
        const qreal dy = pos.y() - center.y();
        qreal dist = qSqrt(dx * dx + dy * dy);
        if (dist < 0.001)
        {
            dist = 0.0;
        }
        const qreal sat = qBound(0.0, dist / radius, 1.0);
        qreal hue = qAtan2(dy, dx) / (2.0 * M_PI);
        if (hue < 0.0)
        {
            hue += 1.0;
        }
        m_hue = hue;
        m_saturation = sat;
        update();
        if (m_onColorChanged)
        {
            m_onColorChanged(color());
        }
    }

    qreal m_hue = 0.05;
    qreal m_saturation = 1.0;
    qreal m_value = 0.96;
    std::function<void(const QColor &)> m_onColorChanged;
};

GroupEditorDialog::GroupEditorDialog(const RegisterGroup &group, QWidget *parent)
    : QDialog(parent), m_group(group), m_selectedColor(group.color)
{
    setWindowTitle(group.id.isEmpty() ? QStringLiteral("新增分组") : QStringLiteral("编辑分组"));
    setObjectName(QStringLiteral("groupEditorDialog"));
    resize(460, 460);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(8);

    auto *nameLabel = new QLabel(QStringLiteral("分组名称"), this);
    m_nameEdit = new QLineEdit(group.name, this);
    m_nameEdit->setPlaceholderText(QStringLiteral("例如:电能参数"));
    m_nameEdit->setMaxLength(40);

    auto *colorLabel = new QLabel(QStringLiteral("颜色标记"), this);

    auto *colorRow = new QHBoxLayout;
    colorRow->setSpacing(14);

    m_colorWheel = new ColorWheelWidget(this);
    m_colorWheel->setColorChangedCallback([this](const QColor &color)
    {
        onColorPicked(color);
    });

    auto *sideBox = new QVBoxLayout;
    sideBox->setSpacing(10);

    auto *previewRow = new QHBoxLayout;
    previewRow->setSpacing(10);
    m_colorPreview = new QLabel(this);
    m_colorPreview->setObjectName(QStringLiteral("groupColorPreview"));
    m_colorPreview->setFixedSize(36, 36);
    m_colorValueLabel = new QLabel(this);
    m_colorValueLabel->setObjectName(QStringLiteral("groupColorValueLabel"));
    previewRow->addWidget(m_colorPreview, 0, Qt::AlignVCenter);
    previewRow->addWidget(m_colorValueLabel, 1, Qt::AlignVCenter);

    auto *valueLabel = new QLabel(QStringLiteral("明度"), this);
    valueLabel->setObjectName(QStringLiteral("groupColorValueCaption"));
    m_valueSlider = new QSlider(Qt::Horizontal, this);
    m_valueSlider->setObjectName(QStringLiteral("groupColorValueSlider"));
    m_valueSlider->setRange(15, 100);
    m_valueSlider->setValue(96);

    sideBox->addLayout(previewRow);
    sideBox->addWidget(valueLabel);
    sideBox->addWidget(m_valueSlider);
    sideBox->addStretch();

    colorRow->addWidget(m_colorWheel, 0, Qt::AlignTop);
    colorRow->addLayout(sideBox, 1);

    auto *descLabel = new QLabel(QStringLiteral("描述 (可选)"), this);
    m_descEdit = new QTextEdit(group.description, this);
    m_descEdit->setPlaceholderText(QStringLiteral("备注该分组的用途"));
    m_descEdit->setMaximumHeight(80);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    if (auto *ok = buttons->button(QDialogButtonBox::Ok))
    {
        ok->setObjectName(QStringLiteral("primaryButton"));
        ok->setText(QStringLiteral("确定"));
    }
    if (auto *cancel = buttons->button(QDialogButtonBox::Cancel))
    {
        cancel->setText(QStringLiteral("取消"));
    }

    layout->addWidget(nameLabel);
    layout->addWidget(m_nameEdit);
    layout->addSpacing(4);
    layout->addWidget(colorLabel);
    layout->addLayout(colorRow);
    layout->addSpacing(4);
    layout->addWidget(descLabel);
    layout->addWidget(m_descEdit);
    layout->addStretch();
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_valueSlider, &QSlider::valueChanged, this, [this](int value)
    {
        if (m_colorWheel)
        {
            m_colorWheel->setValue(value / 100.0);
        }
    });

    QColor initial = QColor(m_selectedColor);
    if (!initial.isValid())
    {
        initial = QColor(QStringLiteral("#f54e00"));
        m_selectedColor = colorToHex(initial);
    }
    m_colorWheel->setColor(initial);
    m_valueSlider->blockSignals(true);
    m_valueSlider->setValue(qBound(15, qRound(m_colorWheel->value() * 100.0), 100));
    m_valueSlider->blockSignals(false);
    updateColorPreview();

    m_nameEdit->setFocus();
}

RegisterGroup GroupEditorDialog::group() const
{
    RegisterGroup result = m_group;
    result.name = m_nameEdit->text().trimmed();
    result.color = m_selectedColor;
    result.description = m_descEdit->toPlainText().trimmed();
    return result;
}

void GroupEditorDialog::onColorPicked(const QColor &color)
{
    if (!color.isValid())
    {
        return;
    }
    m_selectedColor = colorToHex(color);
    if (m_valueSlider)
    {
        const int sliderValue = qBound(15, qRound(color.valueF() * 100.0), 100);
        if (m_valueSlider->value() != sliderValue)
        {
            m_valueSlider->blockSignals(true);
            m_valueSlider->setValue(sliderValue);
            m_valueSlider->blockSignals(false);
        }
    }
    updateColorPreview();
}

void GroupEditorDialog::updateColorPreview()
{
    const QColor color(m_selectedColor);
    if (m_colorPreview)
    {
        m_colorPreview->setStyleSheet(
            QStringLiteral(
                "QLabel#groupColorPreview {"
                "  background: %1;"
                "  border-radius: 18px;"
                "  border: 2px solid rgba(38,37,30,40);"
                "}").arg(color.isValid() ? color.name() : QStringLiteral("#f54e00")));
    }
    if (m_colorValueLabel)
    {
        m_colorValueLabel->setText(
            color.isValid() ? color.name().toUpper() : QStringLiteral("#F54E00"));
    }
}
