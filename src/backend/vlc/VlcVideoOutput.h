#pragma once

#include <QImage>
#include <QMutex>
#include <QObject>
#include <atomic>

struct libvlc_media_player_t;

namespace neovlc::backend {

class VlcMediaPlayer;

// Renders the player's video into an in-memory RV32 buffer and emits each
// completed frame. Must be destroyed before the VlcMediaPlayer it renders.
class VlcVideoOutput final : public QObject
{
    Q_OBJECT

public:
    explicit VlcVideoOutput(VlcMediaPlayer &player, QObject *parent = nullptr);
    ~VlcVideoOutput() override;

    quint64 mediaGeneration() const;
    QImage takeLatestFrame(quint64 *generation = nullptr);

signals:
    void frameAvailable();

private:
    static unsigned onFormat(void **opaque, char *chroma, unsigned *width,
                             unsigned *height, unsigned *pitches, unsigned *lines);
    static void onCleanup(void *opaque);
    static void *onLock(void *opaque, void **planes);
    static void onUnlock(void *opaque, void *picture, void *const *planes);
    static void onDisplay(void *opaque, void *picture);

    void resetSession();
    void releaseBuffer();

    libvlc_media_player_t *m_player = nullptr;
    QMutex m_mutex;
    uchar *m_data = nullptr;
    int m_width = 0;
    int m_height = 0;
    int m_pitch = 0;

    QMutex m_frameMutex;
    QImage m_pendingFrame;
    quint64 m_pendingGeneration = 0;
    QImage m_latestFrame;
    quint64 m_latestGeneration = 0;
    std::atomic<bool> m_frameNotificationPending{false};
    std::atomic<quint64> m_mediaGeneration{0};
};

} // namespace neovlc::backend
