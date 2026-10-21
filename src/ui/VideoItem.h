#pragma once

#include <QImage>
#include <QMetaObject>
#include <QQuickItem>
#include <QtQml/qqmlregistration.h>
#include <vlc/VlcVideoOutput.h>

class QSGNode;

namespace neovlc::ui {

// Draws frames from a VlcVideoOutput into the Qt Quick scene graph, keeping
// the source aspect ratio. The video output must outlive this item.
class VideoItem : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(neovlc::backend::VlcVideoOutput *output READ output WRITE setOutput NOTIFY outputChanged)

public:
    explicit VideoItem(QQuickItem *parent = nullptr);

    neovlc::backend::VlcVideoOutput *output() const { return m_output; }
    void setOutput(neovlc::backend::VlcVideoOutput *output);

    Q_INVOKABLE void clear();

signals:
    void outputChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updateData) override;

private:
    void onFrameAvailable();

    neovlc::backend::VlcVideoOutput *m_output = nullptr;
    QMetaObject::Connection m_connection;
    QImage m_frame;
};

} // namespace neovlc::ui
