#include "VlcVideoOutput.h"

#include "VlcMediaPlayer.h"

#include <QtGlobal>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <vlc/vlc.h>

namespace neovlc::backend {

namespace {

constexpr int kBufferAlignment = 32;

int alignedPitch(int width)
{
    const qint64 bytes = static_cast<qint64>(width) * 4;
    if (bytes <= 0 || bytes > INT_MAX - (kBufferAlignment - 1))
        return 0;
    return static_cast<int>((bytes + kBufferAlignment - 1) & ~(kBufferAlignment - 1));
}

} // namespace

VlcVideoOutput::VlcVideoOutput(VlcMediaPlayer &player, QObject *parent)
    : QObject(parent), m_player(player.player())
{
    // Invalidate frames synchronously with the media switch, whichever thread
    // opened the file. resetSession() only uses atomics and the frame mutex.
    connect(&player, &VlcMediaPlayer::mediaOpened, this,
            &VlcVideoOutput::resetSession, Qt::DirectConnection);

    if (!m_player)
        return;

    libvlc_video_set_callbacks(m_player, &VlcVideoOutput::onLock,
                               &VlcVideoOutput::onUnlock,
                               &VlcVideoOutput::onDisplay, this);
    libvlc_video_set_format_callbacks(m_player, &VlcVideoOutput::onFormat,
                                      &VlcVideoOutput::onCleanup);
}

VlcVideoOutput::~VlcVideoOutput()
{
    // Stopping synchronously joins the video output thread, so no callback can
    // still be running (or start) once the sink's state is released below.
    if (m_player) {
        libvlc_media_player_stop(m_player);

        // libVLC 3 has no API to unregister vmem callbacks. Reset them so a
        // reused player cannot call into this destroyed output. Without a lock
        // callback libVLC refuses to initialize vmem instead of crashing.
        libvlc_video_set_callbacks(m_player, nullptr, nullptr, nullptr, nullptr);
        libvlc_video_set_format_callbacks(m_player, nullptr, nullptr);
    }
    releaseBuffer();
}

quint64 VlcVideoOutput::mediaGeneration() const
{
    return m_mediaGeneration.load(std::memory_order_acquire);
}

QImage VlcVideoOutput::takeLatestFrame(quint64 *generation)
{
    QMutexLocker locker(&m_frameMutex);
    QImage frame = m_latestFrame;
    m_latestFrame = QImage();
    if (generation)
        *generation = m_latestGeneration;
    m_latestGeneration = 0;
    m_frameNotificationPending.store(false, std::memory_order_release);
    return frame;
}

void VlcVideoOutput::resetSession()
{
    m_mediaGeneration.fetch_add(1, std::memory_order_acq_rel);

    QMutexLocker locker(&m_frameMutex);
    m_pendingFrame = QImage();
    m_pendingGeneration = 0;
    m_latestFrame = QImage();
    m_latestGeneration = 0;
    m_frameNotificationPending.store(false, std::memory_order_release);
}

unsigned VlcVideoOutput::onFormat(void **opaque, char *chroma, unsigned *width,
                                  unsigned *height, unsigned *pitches, unsigned *lines)
{
    auto *self = static_cast<VlcVideoOutput *>(*opaque);
    if (!self || !width || !height || !pitches || !lines)
        return 0;

    // The vout may propose decoder-aligned dimensions with extra padding
    // lines. Ask the player for the visible size so the frame is not stretched.
    // The vout may also have applied rotation metadata, so keep its orientation
    // when mapping the unrotated track dimensions onto the output format.
    unsigned visibleWidth = 0;
    unsigned visibleHeight = 0;
    libvlc_video_get_size(self->m_player, 0, &visibleWidth, &visibleHeight);
    if (visibleWidth > 0 && visibleHeight > 0) {
        const bool proposedLandscape = *width >= *height;
        const bool visibleLandscape = visibleWidth >= visibleHeight;
        if (proposedLandscape == visibleLandscape) {
            *width = visibleWidth;
            *height = visibleHeight;
        } else {
            *width = visibleHeight;
            *height = visibleWidth;
        }
    }

    const int frameWidth = static_cast<int>(*width);
    const int frameHeight = static_cast<int>(*height);
    const int pitch = alignedPitch(frameWidth);
    if (frameWidth <= 0 || frameHeight <= 0 || pitch == 0)
        return 0;

    const qint64 size = static_cast<qint64>(pitch) * frameHeight;
    if (size <= 0)
        return 0;

    std::memcpy(chroma, "RV32", 4);

    QMutexLocker locker(&self->m_mutex);
    std::free(self->m_data);
    self->m_data = static_cast<uchar *>(
        std::aligned_alloc(kBufferAlignment, static_cast<size_t>(size)));
    if (!self->m_data) {
        self->m_width = 0;
        self->m_height = 0;
        self->m_pitch = 0;
        return 0;
    }

    self->m_width = frameWidth;
    self->m_height = frameHeight;
    self->m_pitch = pitch;
    pitches[0] = static_cast<unsigned>(pitch);
    lines[0] = static_cast<unsigned>(frameHeight);
    return 1;
}

void VlcVideoOutput::onCleanup(void *opaque)
{
    auto *self = static_cast<VlcVideoOutput *>(opaque);
    if (self)
        self->releaseBuffer();
}

void *VlcVideoOutput::onLock(void *opaque, void **planes)
{
    auto *self = static_cast<VlcVideoOutput *>(opaque);
    if (!self || !planes)
        return nullptr;

    self->m_mutex.lock();
    if (!self->m_data) {
        self->m_mutex.unlock();
        return nullptr;
    }

    planes[0] = self->m_data;
    return self->m_data;
}

void VlcVideoOutput::onUnlock(void *opaque, void *picture, void *const *planes)
{
    Q_UNUSED(picture)
    Q_UNUSED(planes)

    auto *self = static_cast<VlcVideoOutput *>(opaque);
    if (!self)
        return;

    const quint64 generation =
        self->m_mediaGeneration.load(std::memory_order_acquire);

    // The mutex is held from onLock() until here, so copying the completed
    // frame cannot race with decoding writing into the shared buffer.
    QImage frame;
    if (self->m_data) {
        frame = QImage(self->m_data, self->m_width, self->m_height,
                       self->m_pitch, QImage::Format_RGB32)
                    .copy();
    }
    self->m_mutex.unlock();

    if (frame.isNull())
        return;

    QMutexLocker locker(&self->m_frameMutex);
    if (generation != self->m_mediaGeneration.load(std::memory_order_acquire))
        return;
    self->m_pendingFrame = frame;
    self->m_pendingGeneration = generation;
}

void VlcVideoOutput::onDisplay(void *opaque, void *picture)
{
    Q_UNUSED(picture)

    auto *self = static_cast<VlcVideoOutput *>(opaque);
    if (!self)
        return;

    bool notify = false;
    {
        QMutexLocker locker(&self->m_frameMutex);
        if (self->m_pendingFrame.isNull())
            return;
        if (self->m_pendingGeneration
            != self->m_mediaGeneration.load(std::memory_order_acquire)) {
            self->m_pendingFrame = QImage();
            return;
        }

        // Keep only the newest frame; a single queued notification is enough
        // to make the GUI item pick it up.
        self->m_latestFrame = self->m_pendingFrame;
        self->m_latestGeneration = self->m_pendingGeneration;
        self->m_pendingFrame = QImage();
        self->m_pendingGeneration = 0;
        notify = !self->m_frameNotificationPending.exchange(
            true, std::memory_order_acq_rel);
    }

    if (notify)
        emit self->frameAvailable();
}

void VlcVideoOutput::releaseBuffer()
{
    QMutexLocker locker(&m_mutex);
    std::free(m_data);
    m_data = nullptr;
    m_width = 0;
    m_height = 0;
    m_pitch = 0;
}

} // namespace neovlc::backend
