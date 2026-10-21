#include "VideoItem.h"

#include <vlc/VlcVideoOutput.h>

#include <QQuickWindow>
#include <QSGSimpleTextureNode>
#include <QSGTexture>

namespace neovlc::ui {

VideoItem::VideoItem(QQuickItem *parent) : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
}

void VideoItem::setOutput(backend::VlcVideoOutput *output)
{
    if (m_output == output)
        return;

    disconnect(m_connection);
    m_output = output;
    if (m_output) {
        m_connection = connect(m_output, &backend::VlcVideoOutput::frameAvailable,
                               this, &VideoItem::onFrameAvailable,
                               Qt::QueuedConnection);
    }
    emit outputChanged();
}

void VideoItem::clear()
{
    m_frame = QImage();
    update();
}

void VideoItem::onFrameAvailable()
{
    if (!m_output)
        return;

    quint64 generation = 0;
    const QImage frame = m_output->takeLatestFrame(&generation);
    if (frame.isNull() || generation != m_output->mediaGeneration())
        return;

    m_frame = frame;
    update();
}

QSGNode *VideoItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updateData)
{
    Q_UNUSED(updateData)

    if (m_frame.isNull() || !window()) {
        delete oldNode;
        return nullptr;
    }

    auto *node = static_cast<QSGSimpleTextureNode *>(oldNode);
    if (!node) {
        node = new QSGSimpleTextureNode;
        node->setOwnsTexture(true);
        node->setFiltering(QSGTexture::Linear);
    }

    QSGTexture *texture = window()->createTextureFromImage(m_frame);
    if (!texture) {
        delete node;
        return nullptr;
    }
    node->setTexture(texture);

    const QSizeF sourceSize(m_frame.width(), m_frame.height());
    const QSizeF targetSize = size();
    const qreal scale = qMin(targetSize.width() / sourceSize.width(),
                             targetSize.height() / sourceSize.height());
    const QSizeF paintedSize = sourceSize * scale;
    const QPointF offset((targetSize.width() - paintedSize.width()) / 2.0,
                         (targetSize.height() - paintedSize.height()) / 2.0);
    node->setRect(QRectF(offset, paintedSize));

    return node;
}

} // namespace neovlc::ui
