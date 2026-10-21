#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <atomic>
#include <memory>

struct libvlc_event_t;
struct libvlc_media_player_t;

namespace neovlc::backend {

class VlcContext;

// Owns one libVLC media player; must be destroyed before its VlcContext.
class VlcMediaPlayer final : public QObject
{
    Q_OBJECT

public:
    enum PlaybackState {
        StoppedState,
        OpeningState,
        PlayingState,
        PausedState,
        EndedState,
        ErrorState
    };
    Q_ENUM(PlaybackState)

    Q_PROPERTY(bool available READ isAvailable CONSTANT)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    Q_PROPERTY(QString mediaPath READ mediaPath NOTIFY mediaPathChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(PlaybackState playbackState READ playbackState NOTIFY playbackStateChanged)
    Q_PROPERTY(QString playbackStateName READ playbackStateName NOTIFY playbackStateChanged)
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)

    explicit VlcMediaPlayer(VlcContext &context, QObject *parent = nullptr);
    ~VlcMediaPlayer() override;

    bool isAvailable() const { return m_player != nullptr; }
    QString errorString() const { return m_errorString; }
    QString mediaPath() const { return m_mediaPath; }
    int volume() const { return m_volume; }
    PlaybackState playbackState() const { return m_playbackState; }
    QString playbackStateName() const;
    qint64 position() const { return m_position; }
    qint64 duration() const { return m_duration; }
    libvlc_media_player_t *player() const { return m_player.get(); }

    // Opens a local media file. Returns true once the media descriptor is
    // assigned; playability is validated later during parsing and playback.
    Q_INVOKABLE bool openFile(const QUrl &url);

    Q_INVOKABLE bool play();
    Q_INVOKABLE bool pause();
    Q_INVOKABLE bool stop();
    Q_INVOKABLE bool seek(qint64 milliseconds);
    Q_INVOKABLE void setVolume(int volume);

signals:
    void errorStringChanged();
    void mediaPathChanged();
    void mediaOpened();
    void volumeChanged();
    void playbackStateChanged();
    void positionChanged();
    void durationChanged();

private:
    friend class VlcContext;

    struct PlayerDeleter {
        void operator()(libvlc_media_player_t *player) const;
    };

    static void onVlcEvent(const libvlc_event_t *event, void *opaque);
    void handleVlcEvent(int type, qint64 time, qint64 length, quint64 session);
    void attachEvents();
    void detachEvents();

    void setErrorString(const QString &error);
    void setPlaybackState(PlaybackState state);
    void setPosition(qint64 position);
    void setDuration(qint64 duration);
    bool requireMedia(const QString &action);

    VlcContext *m_context;
    std::unique_ptr<libvlc_media_player_t, PlayerDeleter> m_player;
    QString m_errorString;
    QString m_mediaPath;
    int m_volume = 100;
    PlaybackState m_playbackState = StoppedState;
    qint64 m_position = 0;
    qint64 m_duration = 0;
    std::atomic<quint64> m_session{0};
};

} // namespace neovlc::backend
