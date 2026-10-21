#include "VlcMediaPlayer.h"

#include "VlcContext.h"

#include <QFile>
#include <QFileInfo>
#include <QMetaObject>
#include <QUrl>
#include <QtGlobal>
#include <vlc/vlc.h>

namespace neovlc::backend {

namespace {

constexpr libvlc_event_e kPlayerEvents[] = {
    libvlc_MediaPlayerOpening,
    libvlc_MediaPlayerPlaying,
    libvlc_MediaPlayerPaused,
    libvlc_MediaPlayerStopped,
    libvlc_MediaPlayerEndReached,
    libvlc_MediaPlayerEncounteredError,
    libvlc_MediaPlayerTimeChanged,
    libvlc_MediaPlayerLengthChanged,
};

} // namespace

VlcMediaPlayer::VlcMediaPlayer(VlcContext &context, QObject *parent)
    : QObject(parent), m_context(nullptr)
{
    if (!context.isAvailable()) {
        setErrorString(context.errorString().isEmpty()
            ? QStringLiteral("Konteks libVLC tidak tersedia; pemutar media tidak dapat dibuat.")
            : context.errorString());
        return;
    }

    m_player.reset(libvlc_media_player_new(context.instance()));
    if (!m_player) {
        const char *error = libvlc_errmsg();
        setErrorString(error
            ? QString::fromUtf8(error)
            : QStringLiteral("Gagal membuat pemutar media libVLC."));
        return;
    }

    // Only track the context once registration establishes the invariant that
    // every registered player can be detached before the context is destroyed.
    context.registerPlayer(this);
    m_context = &context;
    attachEvents();
}

VlcMediaPlayer::~VlcMediaPlayer()
{
    detachEvents();
    if (m_context)
        m_context->unregisterPlayer(this);
    if (m_player)
        libvlc_media_player_stop(m_player.get());
}

bool VlcMediaPlayer::openFile(const QUrl &url)
{
    if (!m_player || !m_context) {
        setErrorString(QStringLiteral("Pemutar media tidak tersedia."));
        return false;
    }

    QString path;
    if (url.isLocalFile())
        path = url.toLocalFile();
    else if (url.scheme().isEmpty())
        path = url.path();
    else {
        setErrorString(QStringLiteral("Hanya file lokal yang didukung: %1").arg(url.toString()));
        return false;
    }

    if (path.isEmpty()) {
        setErrorString(QStringLiteral("Path media kosong."));
        return false;
    }

    const QFileInfo fileInfo(path);
    if (!fileInfo.isFile() || !fileInfo.isReadable()) {
        setErrorString(QStringLiteral("File tidak ditemukan atau tidak dapat dibaca: %1").arg(path));
        return false;
    }

    const QByteArray encodedPath = QFile::encodeName(path);
    libvlc_media_t *media = libvlc_media_new_path(m_context->instance(), encodedPath.constData());
    if (!media) {
        const char *error = libvlc_errmsg();
        setErrorString(error
            ? QString::fromUtf8(error)
            : QStringLiteral("Gagal membuka file media: %1").arg(path));
        return false;
    }

    libvlc_media_player_set_media(m_player.get(), media);
    libvlc_media_release(media);
    m_session.fetch_add(1, std::memory_order_acq_rel);

    setPlaybackState(StoppedState);
    setPosition(0);
    setDuration(0);
    setErrorString(QString());
    if (m_mediaPath != path) {
        m_mediaPath = path;
        emit mediaPathChanged();
    }
    emit mediaOpened();
    return true;
}

bool VlcMediaPlayer::play()
{
    if (!requireMedia(QStringLiteral("memutar")))
        return false;

    if (libvlc_media_player_play(m_player.get()) != 0) {
        const char *error = libvlc_errmsg();
        setErrorString(error
            ? QString::fromUtf8(error)
            : QStringLiteral("Gagal memulai pemutaran."));
        return false;
    }

    libvlc_audio_set_volume(m_player.get(), m_volume);
    setErrorString(QString());
    return true;
}

bool VlcMediaPlayer::pause()
{
    if (!requireMedia(QStringLiteral("menjeda")))
        return false;

    libvlc_media_player_set_pause(m_player.get(), 1);
    return true;
}

bool VlcMediaPlayer::stop()
{
    if (!m_player) {
        setErrorString(QStringLiteral("Pemutar media tidak tersedia."));
        return false;
    }

    libvlc_media_player_stop(m_player.get());
    m_session.fetch_add(1, std::memory_order_acq_rel);
    setPlaybackState(StoppedState);
    setPosition(0);
    return true;
}

bool VlcMediaPlayer::seek(qint64 milliseconds)
{
    if (!requireMedia(QStringLiteral("melakukan seek")))
        return false;

    if (milliseconds < 0) {
        setErrorString(QStringLiteral("Posisi seek tidak valid."));
        return false;
    }

    libvlc_media_player_set_time(m_player.get(), milliseconds);
    return true;
}

void VlcMediaPlayer::setVolume(int volume)
{
    const int clamped = qBound(0, volume, 200);
    if (m_volume == clamped)
        return;

    m_volume = clamped;
    if (m_player)
        libvlc_audio_set_volume(m_player.get(), m_volume);
    emit volumeChanged();
}

QString VlcMediaPlayer::playbackStateName() const
{
    switch (m_playbackState) {
    case StoppedState:
        return QStringLiteral("stopped");
    case OpeningState:
        return QStringLiteral("opening");
    case PlayingState:
        return QStringLiteral("playing");
    case PausedState:
        return QStringLiteral("paused");
    case EndedState:
        return QStringLiteral("ended");
    case ErrorState:
        return QStringLiteral("error");
    }
    return QStringLiteral("unknown");
}

void VlcMediaPlayer::attachEvents()
{
    libvlc_event_manager_t *manager = libvlc_media_player_event_manager(m_player.get());
    if (!manager)
        return;

    for (libvlc_event_e event : kPlayerEvents)
        libvlc_event_attach(manager, event, &VlcMediaPlayer::onVlcEvent, this);
}

void VlcMediaPlayer::detachEvents()
{
    if (!m_player)
        return;

    libvlc_event_manager_t *manager = libvlc_media_player_event_manager(m_player.get());
    if (!manager)
        return;

    for (libvlc_event_e event : kPlayerEvents)
        libvlc_event_detach(manager, event, &VlcMediaPlayer::onVlcEvent, this);
}

void VlcMediaPlayer::onVlcEvent(const libvlc_event_t *event, void *opaque)
{
    auto *self = static_cast<VlcMediaPlayer *>(opaque);
    if (!self || !event)
        return;

    qint64 time = 0;
    qint64 length = 0;
    switch (event->type) {
    case libvlc_MediaPlayerTimeChanged:
        time = event->u.media_player_time_changed.new_time;
        break;
    case libvlc_MediaPlayerLengthChanged:
        length = event->u.media_player_length_changed.new_length;
        break;
    default:
        break;
    }

    const int type = event->type;
    const quint64 session = self->m_session.load(std::memory_order_acquire);
    QMetaObject::invokeMethod(self, [self, type, time, length, session] {
        self->handleVlcEvent(type, time, length, session);
    }, Qt::QueuedConnection);
}

void VlcMediaPlayer::handleVlcEvent(int type, qint64 time, qint64 length,
                                    quint64 session)
{
    if (session != m_session.load(std::memory_order_acquire))
        return;

    switch (type) {
    case libvlc_MediaPlayerOpening:
        setPlaybackState(OpeningState);
        break;
    case libvlc_MediaPlayerPlaying:
        setPlaybackState(PlayingState);
        break;
    case libvlc_MediaPlayerPaused:
        setPlaybackState(PausedState);
        break;
    case libvlc_MediaPlayerStopped:
        // Keep the error visible until new media is opened or stop() is called
        // explicitly; libVLC emits Stopped after playback failure.
        if (m_playbackState != ErrorState)
            setPlaybackState(StoppedState);
        setPosition(0);
        break;
    case libvlc_MediaPlayerEndReached:
        setPlaybackState(EndedState);
        break;
    case libvlc_MediaPlayerEncounteredError:
        setErrorString(QStringLiteral("Terjadi kesalahan saat memutar media."));
        setPlaybackState(ErrorState);
        break;
    case libvlc_MediaPlayerTimeChanged:
        setPosition(time);
        break;
    case libvlc_MediaPlayerLengthChanged:
        setDuration(length);
        break;
    default:
        break;
    }
}

void VlcMediaPlayer::setPlaybackState(PlaybackState state)
{
    if (m_playbackState == state)
        return;

    m_playbackState = state;
    emit playbackStateChanged();
}

void VlcMediaPlayer::setPosition(qint64 position)
{
    if (m_position == position)
        return;

    m_position = position;
    emit positionChanged();
}

void VlcMediaPlayer::setDuration(qint64 duration)
{
    if (m_duration == duration)
        return;

    m_duration = duration;
    emit durationChanged();
}

void VlcMediaPlayer::setErrorString(const QString &error)
{
    if (m_errorString == error)
        return;
    m_errorString = error;
    emit errorStringChanged();
}

bool VlcMediaPlayer::requireMedia(const QString &action)
{
    if (!m_player) {
        setErrorString(QStringLiteral("Pemutar media tidak tersedia."));
        return false;
    }

    if (m_mediaPath.isEmpty()) {
        setErrorString(QStringLiteral("Tidak ada media untuk %1.").arg(action));
        return false;
    }

    return true;
}

void VlcMediaPlayer::PlayerDeleter::operator()(libvlc_media_player_t *player) const
{
    libvlc_media_player_release(player);
}

} // namespace neovlc::backend
